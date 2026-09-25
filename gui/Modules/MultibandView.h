#pragma once

#include "../../PluginProcessor.h"
#include "SpectrumAnalyser.h"
#include "Panel.h"
#include "LightButton.h"
#include "../../dsp/EffectInfos.h"

class MultibandView : public juce::Component,
                      public juce::TooltipClient,
                      private juce::Timer
{
public:
    explicit MultibandView (AudioPluginAudioProcessor& p)
        : collector (p.getScopeDataCollector()), apvts (p.treeState)
    {
        auto find = [&p] (const ParamIDs::ParameterInfo& info) { return p.treeState.getParameter (info.getParamID()); };

        for (int slot = 0; slot < ParamIDs::numBands; ++slot)
        {
            if (slot < ParamIDs::numBands - 1)
                slotCrossovers[(size_t) slot] = dynamic_cast<juce::AudioParameterFloat*> (find (*ParamIDs::crossoverParams[slot]));

            // a band's level is its slot's out gain, the same one as the OUT under the distortion box
            slotGains[(size_t) slot] = dynamic_cast<juce::AudioParameterFloat*> (
                p.treeState.getParameter (paramIdFor (SlotId { ModuleId::main, slot }, ParamIDs::slotOutGain).getParamID()));
            slotMutes[(size_t) slot] = dynamic_cast<juce::AudioParameterBool*> (find (*ParamIDs::bandMutes[slot]));
            slotSolos[(size_t) slot] = dynamic_cast<juce::AudioParameterBool*> (find (*ParamIDs::bandSolos[slot]));
            slotPowers[(size_t) slot] = dynamic_cast<juce::AudioParameterBool*> (
                p.treeState.getParameter (SlotId { ModuleId::main, slot }.enabled().getParamID()));
            slotTypes[(size_t) slot] = dynamic_cast<juce::AudioParameterChoice*> (
                p.treeState.getParameter (SlotId { ModuleId::main, slot }.type().getParamID()));
            slotInGains[(size_t) slot] = dynamic_cast<juce::AudioParameterFloat*> (
                p.treeState.getParameter (paramIdFor (SlotId { ModuleId::main, slot }, ParamIDs::slotInGain).getParamID()));
        }

        bandCount = dynamic_cast<juce::AudioParameterInt*> (find (ParamIDs::bandCount));
        jassert (bandCount != nullptr);

        auto allFound = [] (const auto& params) { return std::none_of (params.begin(), params.end(), [] (auto* param) { return param == nullptr; }); };
        jassert (allFound (slotCrossovers) && allFound (slotGains) && allFound (slotMutes) && allFound (slotSolos) && allFound (slotPowers)
                 && allFound (slotTypes) && allFound (slotInGains));
        juce::ignoreUnused (allFound);

        setLayout (MainRouting::multibandLayout (2), false);

        startTimerHz (30);
    }

    // one component with many controls, so the tip is for whichever of them is under the mouse right now
    juce::String getTooltip() override
    {
        const auto target = findTarget (getMouseXYRelative().toFloat());
        const auto highPath = exciter && target.index == 1;

        switch (target.kind)
        {
            case Kind::crossover:
                return exciter ? "Where the high passed path starts. Double click to reset"
                               : "Band split frequency. Double click to reset.";
            case Kind::level:
                return juce::String (highPath ? "Gain of the high passed chain" : "Band gain")
                       + ". Linked to distortion's OUT gain.";
            case Kind::power:
                return "Enable / disable audio processing on this band.";
            case Kind::mute:
                return "Mute this band";
            case Kind::solo:
                return "Solo this band. Only soloed bands are heard.";
            case Kind::add:
                return "Click to split a new band off here";
            case Kind::remove:
                return target.index < numBands - 1 ? "Remove the split on this band's right, merging the band there into this one"
                                                    : "Remove the split on this band's left, merging this band into the one there";
            case Kind::drive:
                return "This band's drive, the first control of the distortion it runs. Grabbing it opens the band in the box above. "
                       "Drag up or down, command for fine steps, double click to reset";
            case Kind::slide:
                return "Click, or press " + juce::String (target.index + 1) + ", to edit this "
                       + (exciter ? (highPath ? "high passed chain's" : "full band chain's") : "band's") + " distortion in the box above. "
                       + "Drag sideways to slide it, or up and down to widen or narrow it"
                       + (exciter ? "" : ". Right click to remove it");
            case Kind::none:
                break;
        }

        return {};
    }

    // a click on a band, rather than on one of its controls. passes the band's slot
    std::function<void (int)> onBandSelected;

    /*  Which slot each band runs and which crossover each split is, from the routing. Everything below works
        in bands left to right, so this lines the slots' parameters up in that order. */
    void setLayout (const MainRouting::BandLayout& newLayout, bool showExciter)
    {
        layout = newLayout;
        numBands = juce::jlimit (2, ParamIDs::numBands, layout.numBands);
        exciter = showExciter;

        for (int band = 0; band < numBands; ++band)
        {
            const auto slot = (size_t) layout.slots[(size_t) band];

            gains[(size_t) band] = slotGains[slot];
            inGains[(size_t) band] = slotInGains[slot];
            mutes[(size_t) band] = slotMutes[slot];
            solos[(size_t) band] = slotSolos[slot];
            powers[(size_t) band] = slotPowers[slot];
        }

        for (int edge = 0; edge < numBands - 1; ++edge)
            crossovers[(size_t) edge] = slotCrossovers[(size_t) layout.crossovers[(size_t) edge]];

        setActiveBand (activeSlot);
    }

    // the band showing the slot the box above is editing, or none when this layout doesn't run it
    void setActiveBand (int slot)
    {
        activeSlot = slot;
        activeBand = -1;

        for (int band = 0; band < numBands; ++band)
            if (layout.slots[(size_t) band] == slot)
                activeBand = band;

        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const auto area = getLocalBounds().toFloat();

        // square along the top, and rounded at the bottom to follow the box it sits flush in
        juce::Path outline;
        outline.addRoundedRectangle (area.getX(), area.getY(), area.getWidth(), area.getHeight(),
                                     Panel::boxCornerSize, Panel::boxCornerSize, false, false, true, true);

        g.saveState();
        g.reduceClipRegion (outline);

        g.setColour (theme().multibandBackground);
        g.fillRect (area);

        // each band's in gain behind everything, for reference against its out gain line: just where it's set, not live
        for (int band = 0; band < numBands; ++band)
        {
            const auto span = levelSpan (band);
            const auto y = gainToY (inGains[(size_t) band]->get());

            g.setColour (theme().multibandInGain);
            g.drawLine (span.getStart() + 6.0f, y, span.getEnd() - 6.0f, y, 1.0f);
        }

        spectrum.paint (g, area, collector.getSampleRate(), 1.5f, theme().multibandSpectrumLine, theme().multibandSpectrumFill);

        g.setColour (theme().multibandInputSpectrum);
        g.strokePath (inputSpectrum.makePath (area, collector.getSampleRate()), juce::PathStrokeType (1.0f));

        g.setFont (getLookAndFeel().getPopupMenuFont().withHeight (fontHeight));

        // every band's shading goes down before any band's lines, since lines can run on into their neighbours
        for (int band = 0; band < numBands; ++band)
            paintBandShading (g, band);

        for (int band = 0; band < numBands; ++band)
            paintBand (g, band);

        for (int edge = 0; edge < numBands - 1; ++edge)
            paintCrossover (g, edge);

        if (hover.kind == Kind::add)
            paintAddPreview (g);

        g.restoreState();

        g.setColour (theme().multibandTopEdge);
        g.drawLine (area.getX(), area.getY() + 0.5f, area.getRight(), area.getY() + 0.5f, 1.0f);
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        const auto target = findTarget (e.position);

        // the add preview follows the mouse along the top, not just when it first arrives there
        if (target != hover || target.kind == Kind::add)
        {
            hover = target;
            addX = e.position.x;
            repaint();
        }

        switch (hover.kind)
        {
            case Kind::crossover: setMouseCursor (juce::MouseCursor::LeftRightResizeCursor); break;
            case Kind::slide:     setMouseCursor (juce::MouseCursor::UpDownLeftRightResizeCursor); break;
            case Kind::level:
            case Kind::drive:     setMouseCursor (juce::MouseCursor::UpDownResizeCursor);    break;
            default:              setMouseCursor (juce::MouseCursor::PointingHandCursor);    break;
        }
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        hover = {};
        repaint();
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
        {
            showBandMenu (bandAtX (e.position.x));
            return;
        }

        drag = findTarget (e.position);

        if (drag.kind == Kind::add)
        {
            addSplitAt (SpectrumAnalyser::xToFreq (e.position.x, (float) getWidth()));
            drag = hover = {};
            return;
        }

        // command-click resets, the same as on the knobs
        if (e.mods.isCommandDown() && (drag.kind == Kind::crossover || drag.kind == Kind::level || drag.kind == Kind::drive))
        {
            resetTarget (drag);
            drag = {};
            return;
        }

        switch (drag.kind)
        {
            case Kind::remove: removeBand (drag.index); drag = hover = {}; break;

            // grabbing a band's drive also brings that band up in the box above, to carry on with the rest of it
            case Kind::drive:
                selectBand (drag.index);

                if (auto* param = primaryParam (drag.index))
                {
                    lastDragPosition = e.position;
                    driveNorm = param->getValue();
                    param->beginChangeGesture();
                }
                break;
            /*  A toggle that brings a band into play also brings it up in the box above, to carry on with: solo always,
                mute only when it's coming off, and power only when it's coming on. Silencing a band or switching it
                off leaves the box on whatever it was showing. */
            case Kind::power:
                toggle (powers[(size_t) drag.index]);

                if (powers[(size_t) drag.index]->get())
                    selectBand (drag.index);
                break;

            case Kind::mute:
                toggle (mutes[(size_t) drag.index]);

                if (! mutes[(size_t) drag.index]->get())
                    selectBand (drag.index);
                break;

            case Kind::solo:
                toggle (solos[(size_t) drag.index]);
                selectBand (drag.index);
                break;

            /*  Both drags move by how far the mouse goes rather than to where it is, so grabbing a line off centre
                doesn't make it jump, and neither does command slowing it down partway through. */
            case Kind::crossover:
                lastDragPosition = e.position;
                dragLogFreq = std::log10 ((double) crossovers[(size_t) drag.index]->get());
                crossovers[(size_t) drag.index]->beginChangeGesture();
                break;

            case Kind::level:
                lastDragPosition = e.position;
                levelDelta = 0.0f;

                // every band's gesture is opened, since shift can be pressed partway through the drag
                for (int band = 0; band < numBands; ++band)
                {
                    dragStartGains[(size_t) band] = gains[(size_t) band]->get();
                    gains[(size_t) band]->beginChangeGesture();
                }

                selectBand (drag.index);
                break;

            /*  Anywhere else in a band: a click brings it up in the box above, and a drag moves its own splits
                together, sideways sliding it along the spectrum, up and down widening or narrowing it. The splits'
                gestures only open once it's really a drag, so a click leaves nothing in the host's undo. */
            case Kind::slide:
                lastDragPosition = e.position;
                slideOffset = 0.0;
                slideSpread = 0.0;
                sliding = false;

                for (int edge = 0; edge < numBands - 1; ++edge)
                    slideStartLogs[(size_t) edge] = std::log10 ((double) crossovers[(size_t) edge]->get());

                selectBand (drag.index);
                break;

            case Kind::add:
            case Kind::none: break;
        }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (drag.kind == Kind::drive)
        {
            if (auto* param = primaryParam (drag.index))
            {
                driveNorm = juce::jlimit (0.0f, 1.0f, driveNorm - (e.position.y - lastDragPosition.y) * dragScale (e) / driveDragPixels);
                param->setValueNotifyingHost (driveNorm);
            }

            lastDragPosition = e.position;
            repaint();
            return;
        }

        if (drag.kind == Kind::crossover)
        {
            // kept between its neighbours, the cascaded split stops summing flat once they cross
            auto& param = *crossovers[(size_t) drag.index];
            const auto lowest = drag.index > 0 ? crossovers[(size_t) drag.index - 1]->get() : param.range.start;
            // max'd, since automation or the exciter's slider can already have left them crossed
            const auto highest = juce::jmax (lowest, drag.index < numBands - 2 ? crossovers[(size_t) drag.index + 1]->get() : param.range.end);

            // in log frequency, the same as the x axis, and kept inside the limits so turning back responds straight away
            dragLogFreq += (double) ((e.position.x - lastDragPosition.x) / (float) getWidth() * dragScale (e)) * logRange();
            dragLogFreq = juce::jlimit (std::log10 ((double) lowest), std::log10 ((double) highest), dragLogFreq);

            param = (float) std::pow (10.0, dragLogFreq);
        }
        else if (drag.kind == Kind::slide)
        {
            const auto [left, right] = slideEdges (drag.index);

            if (! sliding)
            {
                if (! e.mouseWasDraggedSinceMouseDown())
                    return;

                sliding = true;

                for (auto edge : { left, right })
                    if (edge >= 0)
                        crossovers[(size_t) edge]->beginChangeGesture();
            }

            const auto& range = crossovers[0]->range;
            const auto bottom = std::log10 ((double) range.start), top = std::log10 ((double) range.end);
            const auto startOf = [this] (int edge) { return slideStartLogs[(size_t) edge]; };

            // sideways slides the band along, up spreads its splits apart and down draws them together
            const auto scale = (double) dragScale (e) * logRange() / (double) getWidth();
            slideOffset += (double) (e.position.x - lastDragPosition.x) * scale;
            slideSpread -= (double) (e.position.y - lastDragPosition.y) * scale * 0.5;

            // each split held between the splits beyond it, or the ends of the range where there are none
            const auto lowest = left > 0 ? startOf (left - 1) : bottom;
            const auto highest = right >= 0 && right < numBands - 2 ? startOf (right + 1) : top;

            auto leftLog = left >= 0 ? juce::jlimit (lowest, highest, startOf (left) + slideOffset - slideSpread) : 0.0;
            auto rightLog = right >= 0 ? juce::jlimit (lowest, highest, startOf (right) + slideOffset + slideSpread) : 0.0;

            // and never closer than a small gap, or drawing them together would cross them over
            if (left >= 0 && right >= 0 && rightLog - leftLog < minSplitGap)
            {
                const auto half = minSplitGap * 0.5;
                const auto centre = juce::jlimit (lowest + half, juce::jmax (lowest + half, highest - half), (leftLog + rightLog) * 0.5);
                leftLog = centre - half;
                rightLog = centre + half;
            }

            // where the splits ended up goes back into the drag's totals, so turning back responds straight away
            if (left >= 0 && right >= 0)
            {
                const auto leftMoved = leftLog - startOf (left), rightMoved = rightLog - startOf (right);
                slideOffset = (leftMoved + rightMoved) * 0.5;
                slideSpread = (rightMoved - leftMoved) * 0.5;
            }
            else if (left >= 0)
            {
                slideOffset = leftLog - startOf (left) + slideSpread;
            }
            else
            {
                slideOffset = rightLog - startOf (right) - slideSpread;
            }

            if (left >= 0)
                *crossovers[(size_t) left] = (float) std::pow (10.0, leftLog);

            if (right >= 0)
                *crossovers[(size_t) right] = (float) std::pow (10.0, rightLog);
        }
        else if (drag.kind == Kind::level)
        {
            const auto start = dragStartGains[(size_t) drag.index];

            levelDelta -= (e.position.y - lastDragPosition.y) * dragScale (e) * (levelCeilingDb - levelFloorDb()) / levelTrack().getHeight();
            levelDelta = juce::jlimit (levelFloorFor (start) - start, levelCeilingFor (start) - start, levelDelta);

            const auto delta = levelDelta;

            /*  With shift every band moves by the same amount, keeping their offsets from each other.
                Letting go of shift mid drag puts the others back where they started. */
            for (int band = 0; band < numBands; ++band)
            {
                const auto moves = band == drag.index || e.mods.isShiftDown();
                const auto from = dragStartGains[(size_t) band];
                *gains[(size_t) band] = juce::jlimit (levelFloorFor (from), levelCeilingFor (from), from + (moves ? delta : 0.0f));
            }
        }
        else
        {
            return;
        }

        lastDragPosition = e.position;
        repaint();
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (drag.kind == Kind::crossover)
            crossovers[(size_t) drag.index]->endChangeGesture();
        else if (drag.kind == Kind::slide && sliding)
            for (auto edge : slideEdges (drag.index))
                if (edge >= 0)
                    crossovers[(size_t) edge]->endChangeGesture();
        else if (drag.kind == Kind::level)
            for (int band = 0; band < numBands; ++band)
                gains[(size_t) band]->endChangeGesture();
        else if (drag.kind == Kind::drive)
            if (auto* param = primaryParam (drag.index))
                param->endChangeGesture();

        drag = {};
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override { resetTarget (findTarget (e.position)); }

private:
    enum class Kind { none, crossover, level, power, mute, solo, remove, drive, slide, add };

    // the crossovers either side of a band, -1 where it runs to the end of the range instead
    std::array<int, 2> slideEdges (int band) const { return { band > 0 ? band - 1 : -1, band < numBands - 1 ? band : -1 }; }

    // the x axis in log frequency
    static double logRange() { return std::log10 (scope_constants::maxDrawFreq) - std::log10 (scope_constants::minDrawFreq); }

    struct Target
    {
        Kind kind = Kind::none;
        int index = -1;

        bool operator!= (const Target& other) const { return kind != other.kind || index != other.index; }
    };

    // band edges run 0 .. numBands, the outer two being the view's own sides
    float edgeX (int edge) const
    {
        if (edge <= 0) return 0.0f;
        if (edge >= numBands) return (float) getWidth();

        return SpectrumAnalyser::freqToX (crossovers[(size_t) edge - 1]->get(), (float) getWidth());
    }

    juce::Rectangle<float> bandArea (int band) const
    {
        const auto left = edgeX (band);
        return { left, 0.0f, juce::jmax (0.0f, edgeX (band + 1) - left), (float) getHeight() };
    }

    // the stretch the level lines travel over, clear of the toggles along the top and the drive readouts along the bottom
    juce::Rectangle<float> levelTrack() const
    {
        return getLocalBounds().toFloat().withTrimmedTop (toggleSize + 8.0f).withTrimmedBottom (bottomMargin + driveRowHeight)
                   .reduced (0.0f, thumbRadius + 2.0f);
    }

    /*  The level lines cover less than the out gain they move. The top stops at +9 dB: past that a band just gets
        loud rather than useful. The bottom is -24 dB for a band split, and the out gain's own floor for the exciter,
        whose high path gets mixed in quietly and wants the finer control of the longer travel. The OUT under the
        box still reaches the full range, and a band already set past an end from there is drawn pinned to that
        edge, and can be dragged back in but not further out. */
    static constexpr float levelCeilingDb = 9.0f;
    static constexpr float bandLevelFloorDb = -24.0f;

    float levelFloorDb() const { return exciter ? gains[0]->range.start : bandLevelFloorDb; }
    float levelFloorFor (float startDb) const { return juce::jmin (levelFloorDb(), startDb); }
    static float levelCeilingFor (float startDb) { return juce::jmax (levelCeilingDb, startDb); }

    // pinned to the track by default, for the lines and thumbs. the high pass curve is left to fall away past its bottom
    float gainToY (float db, bool pinToTrack = true) const
    {
        const auto track = levelTrack();
        const auto y = juce::jmap (db, levelFloorDb(), levelCeilingDb, track.getBottom(), track.getY());

        return pinToTrack ? juce::jlimit (track.getY(), track.getBottom(), y) : juce::jmax (track.getY(), y);
    }

    // the exciter's high path follows its filter rather than sitting flat
    float levelY (int band, float x, bool pinToTrack = true) const
    {
        auto db = gains[(size_t) band]->get();

        if (exciter && band == 1)
            db += highpassDb (SpectrumAnalyser::xToFreq (x, (float) getWidth()));

        return gainToY (db, pinToTrack);
    }

    float highpassDb (double freq) const
    {
        const auto slope = MainRouting::crossoverSlopeFrom (apvts.state.getProperty (MainRouting::crossoverSlopeProperty));
        const auto ratio = std::pow (freq / (double) crossovers[0]->get(), slope / 6.0);
        return juce::Decibels::gainToDecibels ((float) (ratio / (1.0 + ratio)), -100.0f);
    }

    juce::Path highpassCurve() const
    {
        auto pointAt = [this] (float x) { return juce::Point<float> (x, juce::jmin (levelY (1, x, false), (float) getHeight())); };

        juce::Path curve;
        curve.startNewSubPath (pointAt (0.0f));

        for (auto x = 2.0f; x <= (float) getWidth(); x += 2.0f)
            curve.lineTo (pointAt (x));

        return curve;
    }

    /*  Power, mute, solo in the band's top corner: in a row, or a column once the band is too narrow for
        the row, or nothing once it can't fit even one. */
    juce::Rectangle<float> toggleArea (int band, int which) const
    {
        const auto area = bandArea (band);
        const auto step = (float) which * (toggleSize + toggleGap);

        if (area.getWidth() >= toggleSize * 3.0f + toggleGap * 2.0f + toggleInset * 2.0f)
            return { area.getX() + toggleInset + step, toggleInset, toggleSize, toggleSize };

        if (area.getWidth() >= toggleSize + toggleInset * 2.0f)
            return { area.getX() + toggleInset, toggleInset + step, toggleSize, toggleSize };

        return {};
    }

    Target findTarget (juce::Point<float> pos) const
    {
        for (int band = 0; band < numBands; ++band)
        {
            // each a little bigger to hit than it's drawn; where two overlap the earlier wins, which only ever splits the gap between them
            if (toggleArea (band, 0).expanded (toggleHitMargin).contains (pos)) return { Kind::power, band };
            if (toggleArea (band, 1).expanded (toggleHitMargin).contains (pos)) return { Kind::mute, band };
            if (toggleArea (band, 2).expanded (toggleHitMargin).contains (pos)) return { Kind::solo, band };
            if (removeArea (band).expanded (toggleHitMargin).contains (pos)) return { Kind::remove, band };
            if (driveArea (band).contains (pos)) return { Kind::drive, band };
        }

        // crossovers win where they overlap a level line, a narrow band would otherwise have no split left to grab
        for (int edge = 0; edge < numBands - 1; ++edge)
            if (std::abs (pos.x - edgeX (edge + 1)) <= grabDistance)
                return { Kind::crossover, edge };

        if (canAddAt (pos))
            return { Kind::add, -1 };

        // level lines can run past their own band, so where two are in reach the nearer one wins
        auto nearestLevel = Target {};
        auto nearestDistance = grabDistance;

        for (int band = 0; band < numBands; ++band)
        {
            const auto span = levelSpan (band);
            const auto distance = std::abs (pos.y - levelY (band, pos.x));

            if (pos.x >= span.getStart() && pos.x <= span.getEnd() && distance <= nearestDistance)
            {
                nearestLevel = { Kind::level, band };
                nearestDistance = distance;
            }
        }

        if (nearestLevel.kind == Kind::level)
            return nearestLevel;

        // what's left of the band, clear of every control, grabs the band itself
        for (int band = 0; band < numBands; ++band)
            if (bandArea (band).contains (pos))
                return { Kind::slide, band };

        return {};
    }
    
    juce::Range<float> levelSpan (int band) const
    {
        if (exciter && band == 0)
            return { 0.0f, (float) getWidth() };

        const auto area = bandArea (band);
        return { area.getX(), area.getRight() };
    }

    bool isAudible (int band) const
    {
        auto anySolo = false;

        for (int i = 0; i < numBands; ++i)
            anySolo = anySolo || solos[(size_t) i]->get();

        return !mutes[(size_t) band]->get() && (!anySolo || solos[(size_t) band]->get());
    }

    // darkened when it isn't the band being edited, darker still when it's silenced, and lit while its level is handled
    void paintBandShading (juce::Graphics& g, int band)
    {
        const auto area = bandArea (band);

        if (area.isEmpty())
            return;

        // darkened rather than washed out, so the band being edited is the brightest one
        if (band != activeBand)
        {
            g.setColour (theme().multibandInactiveShade);
            g.fillRect (area);
        }

        // silenced by its own mute or someone else's solo
        if (!isAudible (band))
        {
            g.setColour (theme().multibandSilencedShade);
            g.fillRect (area);
        }

        // lit whole while it's being slid, it being the whole band that's grabbed
        if (drag.kind == Kind::slide && drag.index == band && sliding)
        {
            g.setColour (theme().multibandBandHighlight);
            g.fillRect (area);
        }

        const auto heat = hotness (Kind::level, band);

        // the flat lines also get the strip they can be grabbed by
        if (heat > 0.0f)
        {
            g.setColour (theme().multibandBandHighlight.withMultipliedAlpha (heat));
            g.fillRect (area);

            if (!(exciter && band == 1))
            {
                const auto span = levelSpan (band);
                const auto y = levelY (band, area.getCentreX());

                g.setColour (theme().multibandBandHighlight.withMultipliedAlpha (1.6f * heat));
                g.fillRect (juce::Rectangle<float> (span.getStart(), y - grabDistance, span.getLength(), grabDistance * 2.0f));
            }
        }
    }

    void paintBand (juce::Graphics& g, int band)
    {
        const auto area = bandArea (band);

        if (area.isEmpty())
            return;

        const auto active = band == activeBand;
        const auto db = gains[(size_t) band]->get();
        const auto y = levelY (band, area.getCentreX());
        const auto hovered = hotness (Kind::level, band) > 0.0f;
        const auto lineColour = active || hovered ? theme().multibandLevelHighlight : theme().multibandLevel;

        g.setColour (lineColour);

        // the high path's curve runs the full width, it still leaks a little below the split
        if (exciter && band == 1)
            g.strokePath (highpassCurve(), juce::PathStrokeType (hovered ? 2.0f : 1.5f));
        else
            g.drawLine (levelSpan (band).getStart() + 6.0f, y, levelSpan (band).getEnd() - 6.0f, y, hovered ? 2.0f : 1.5f);

        if (exciter)
        {
            g.setColour (theme().multibandLabel);
            g.drawText (band == 0 ? "FULL" : "HIGH", area.reduced (6.0f, 3.0f).withHeight (fontHeight),
                        juce::Justification::centredRight, false);
            g.setColour (lineColour);
        }
        g.fillEllipse (juce::Rectangle<float> (thumbRadius * 2.0f, thumbRadius * 2.0f).withCentre ({ area.getCentreX(), y }));

        if (hovered || !juce::approximatelyEqual (db, 0.0f))
        {
            g.drawText (formatDecibels (db),
                        juce::Rectangle<float> (area.getCentreX() + thumbRadius + 3.0f, y - fontHeight - 2.0f, 50.0f, fontHeight),
                        juce::Justification::centredLeft, false);
        }

        paintRemove (g, band);
        paintDrive (g, band);
        paintPower (g, toggleArea (band, 0), powers[(size_t) band]->get());
        paintToggle (g, toggleArea (band, 1), "M", mutes[(size_t) band]->get(), theme().multibandMute);
        paintToggle (g, toggleArea (band, 2), "S", solos[(size_t) band]->get(), theme().multibandSolo);
    }

    void paintToggle (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& text, bool on, juce::Colour onColour)
    {
        if (area.isEmpty())
            return;

        g.setColour (on ? onColour : theme().multibandButton);
        g.fillRoundedRectangle (area, 3.0f);

        g.setColour (on ? theme().multibandButtonTextOn : theme().multibandButtonText);
        g.drawText (text, area, juce::Justification::centred, false);
    }

    // the band's distortion on or off, drawn like the box's own power button
    void paintPower (juce::Graphics& g, juce::Rectangle<float> area, bool on)
    {
        if (area.isEmpty())
            return;

        g.setColour (theme().multibandButton);
        g.fillRoundedRectangle (area, 3.0f);

        drawGlowingGlyph (g, powerGlyph(), area, on ? theme().powerOn : theme().powerOff);
    }

    void resetTarget (Target target)
    {
        if (target.kind == Kind::crossover)
            resetToDefault (*crossovers[(size_t) target.index], *ParamIDs::crossoverParams[layout.crossovers[(size_t) target.index]]);
        else if (target.kind == Kind::level)
            resetToDefault (*gains[(size_t) target.index], ParamIDs::slotOutGain);
        else if (target.kind == Kind::drive)
            if (auto* param = primaryParam (target.index))
            {
                param->beginChangeGesture();
                param->setValueNotifyingHost (param->getDefaultValue());
                param->endChangeGesture();
            }
    }

    // how strongly a control shows as being handled: fully while dragged, half while only under the mouse
    float hotness (Kind kind, int index) const
    {
        if (drag.kind == kind && drag.index == index)
            return 1.0f;

        return hover.kind == kind && hover.index == index ? 0.5f : 0.0f;
    }

    // the top strip to add a new band
    bool canAddAt (juce::Point<float> pos) const
    {
        if (exciter || numBands >= ParamIDs::numBands || pos.y >= addStripHeight)
            return false;

        // not in a margin round the buttons up there either, so a near miss on one doesn't split a band off
        for (int band = 0; band < numBands; ++band)
        {
            for (int which = 0; which < 3; ++which)
                if (toggleArea (band, which).expanded (addClearance).contains (pos))
                    return false;

            if (removeArea (band).expanded (addClearance).contains (pos))
                return false;
        }

        for (int edge = 1; edge < numBands; ++edge)
            if (std::abs (pos.x - edgeX (edge)) < grabDistance * 2.0f)
                return false;

        return true;
    }

    juce::Rectangle<float> removeArea (int band) const
    {
        if (exciter || numBands <= 2)
            return {};

        const auto area = bandArea (band);
        const auto bin = juce::Rectangle<float> (area.getRight() - toggleInset - toggleSize, toggleInset, toggleSize, toggleSize);

        // room for at least a column of toggles beside it, so it never ends up on top of a split line in a sliver of a band
        const auto roomy = area.getWidth() >= toggleSize * 2.0f + toggleInset * 3.0f;
        const auto clearOfToggles = toggleArea (band, 2).isEmpty() || toggleArea (band, 2).getRight() + toggleGap < bin.getX();

        return roomy && clearOfToggles ? bin : juce::Rectangle<float> {};
    }

    int bandAtX (float x) const
    {
        for (int band = 0; band < numBands; ++band)
            if (x < bandArea (band).getRight())
                return band;

        return numBands - 1;
    }

    // the splits in use, left to right
    std::vector<float> splitFrequencies() const
    {
        std::vector<float> splits;

        for (int edge = 0; edge < numBands - 1; ++edge)
            splits.push_back (crossovers[(size_t) edge]->get());

        return splits;
    }

    void setSplits (std::vector<float> splits)
    {
        std::sort (splits.begin(), splits.end());

        const auto count = (int) splits.size() + 1;
        const auto newLayout = MainRouting::multibandLayout (count);

        for (size_t i = 0; i < splits.size(); ++i)
        {
            auto* param = slotCrossovers[(size_t) newLayout.crossovers[i]];
            param->beginChangeGesture();
            *param = splits[i];
            param->endChangeGesture();
        }

        bandCount->beginChangeGesture();
        *bandCount = count;
        bandCount->endChangeGesture();
    }

    // the new band starts as a plain grill on defaults, rather than whatever the slot it lands on was last left as
    void addSplitAt (double freq)
    {
        const auto before = layout;

        auto splits = splitFrequencies();
        splits.push_back ((float) freq);
        setSplits (splits);

        // one more band always means exactly one slot the old layout didn't run; it's the band that just appeared
        const auto after = MainRouting::multibandLayout ((int) splits.size() + 1);

        for (int band = 0; band < after.numBands; ++band)
        {
            const auto slot = after.slots[(size_t) band];
            const auto wasRunning = std::find (before.slots.begin(), before.slots.begin() + before.numBands, slot)
                                    != before.slots.begin() + before.numBands;

            if (! wasRunning)
                resetSlot (slot);
        }
    }

    void resetSlot (int slotIndex)
    {
        const SlotId slot { ModuleId::main, slotIndex };

        juce::StringArray ids { slot.type().getParamID(), slot.enabled().getParamID(),
                                ParamIDs::bandMutes[slotIndex]->getParamID(), ParamIDs::bandSolos[slotIndex]->getParamID() };

        for (const auto* level : { &ParamIDs::slotInGain, &ParamIDs::slotMix, &ParamIDs::slotOutGain, &ParamIDs::gainLink })
            ids.add (paramIdFor (slot, *level).getParamID());

        for (const auto* typeLayout : EffectInfos::layoutsFor (ModuleId::main))
            for (const auto& descriptor : typeLayout->params)
                if (! descriptor.id.isNull())
                    ids.addIfNotAlreadyThere (paramIdFor (slot, descriptor).getParamID());

        for (const auto& id : ids)
        {
            if (auto* param = apvts.getParameter (id))
            {
                param->beginChangeGesture();
                param->setValueNotifyingHost (param->getDefaultValue());
                param->endChangeGesture();
            }
        }

        apvts.state.removeProperty (waveshapes::groupFilterProperty (slot), nullptr);
    }

    /*  Drops the split on a band's right, the one its bin sits beside, so the band to its right merges left into it.
        The last band has none there, so it drops the one on its left instead. */
    void removeBand (int band)
    {
        if (exciter || numBands <= 2)
            return;

        auto splits = splitFrequencies();
        splits.erase (splits.begin() + (band < numBands - 1 ? band : band - 1));
        setSplits (splits);
    }

    void showBandMenu (int band)
    {
        juce::Component::SafePointer<MultibandView> safeThis (this);

        juce::PopupMenu menu;
        menu.setLookAndFeel (&getLookAndFeel());
        menu.addItem ("Remove this band", ! exciter && numBands > 2, false, [safeThis, band] {
            if (safeThis != nullptr)
                safeThis->removeBand (band);
        });

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition());
    }

    /*  The first control of whatever distortion the band's slot runs - its drive, saturation or strength, macro 0 of
        every distortion's layout - so the bands' drives can be set side by side without going through each box. */
    juce::RangedAudioParameter* primaryParam (int band) const
    {
        const auto slot = layout.slots[(size_t) band];
        const auto* type = slotTypes[(size_t) slot];
        const auto layouts = EffectInfos::layoutsFor (ModuleId::main);

        if (type == nullptr || type->getIndex() < 0 || type->getIndex() >= layouts.count)
            return nullptr;

        const auto& descriptor = layouts.data[type->getIndex()]->params[0];
        return apvts.getParameter (paramIdFor (SlotId { ModuleId::main, slot }, descriptor).getParamID());
    }

    // dB where the control is in dB, otherwise how far along its range it is
    static juce::String driveText (const juce::RangedAudioParameter& param)
    {
        const auto value = param.convertFrom0to1 (param.getValue());
        const auto isDb = param.getNormalisableRange().start < 0.0f;

        return "DRIVE " + (isDb ? formatDecibels (value) : formatPercent (param.getValue()));
    }

    // centred along the band's bottom
    juce::Rectangle<float> driveArea (int band) const
    {
        const auto area = bandArea (band);

        if (area.getWidth() < driveWidth + toggleInset * 2.0f)
            return {};

        return juce::Rectangle<float> (driveWidth, fontHeight + 2.0f)
                   .withCentre ({ area.getCentreX(), (float) getHeight() - bottomMargin - driveRowHeight * 0.5f });
    }

    void paintDrive (juce::Graphics& g, int band)
    {
        const auto area = driveArea (band);
        const auto* param = primaryParam (band);

        if (area.isEmpty() || param == nullptr)
            return;

        const auto heat = hotness (Kind::drive, band);

        if (heat > 0.0f)
            g.setColour (theme().multibandDriveActive.interpolatedWith (theme().multibandDriveHot, heat));
        else
            g.setColour (band == activeBand ? theme().multibandDriveActive : theme().multibandDrive);

        g.drawText (driveText (*param), area, juce::Justification::centred, false);
    }

    // a bin: a lid with a handle, and a body with slats, red while it's under the mouse
    void paintRemove (juce::Graphics& g, int band)
    {
        const auto area = removeArea (band);

        if (area.isEmpty())
            return;

        const auto hot = hover.kind == Kind::remove && hover.index == band;

        g.setColour (theme().multibandButton);
        g.fillRoundedRectangle (area, 3.0f);

        const auto icon = area.reduced (3.0f);
        const auto lidY = icon.getY() + icon.getHeight() * 0.22f;
        const auto body = icon.withTop (lidY + 1.5f).reduced (icon.getWidth() * 0.12f, 0.0f);

        g.setColour (hot ? theme().multibandRemoveHot : theme().multibandRemove);
        g.drawLine (icon.getX(), lidY, icon.getRight(), lidY, 1.2f);
        g.drawLine (icon.getCentreX() - 1.5f, icon.getY(), icon.getCentreX() + 1.5f, icon.getY(), 1.2f);
        g.drawRect (body, 1.0f);

        for (const auto share : { 0.33f, 0.66f })
        {
            const auto slat = body.getX() + body.getWidth() * share;
            g.drawLine (slat, body.getY() + 1.5f, slat, body.getBottom() - 1.5f, 0.8f);
        }
    }

    // where a click on the top strip would put the new split, a grey line down the view and a plus at the top
    void paintAddPreview (juce::Graphics& g)
    {
        g.setColour (theme().multibandAddLine);
        g.drawLine (addX, 0.0f, addX, (float) getHeight(), 1.0f);

        const auto badge = juce::Rectangle<float> (addBadgeSize, addBadgeSize).withCentre ({ addX, addStripHeight * 0.5f });
        g.setColour (theme().multibandAddBadge);
        g.fillEllipse (badge);

        const auto cross = badge.reduced (addBadgeSize * 0.28f);
        g.setColour (theme().multibandAddCross);
        g.drawLine (cross.getX(), cross.getCentreY(), cross.getRight(), cross.getCentreY(), 1.5f);
        g.drawLine (cross.getCentreX(), cross.getY(), cross.getCentreX(), cross.getBottom(), 1.5f);
    }

    void paintCrossover (juce::Graphics& g, int edge)
    {
        const auto x = edgeX (edge + 1);
        const auto heat = hotness (Kind::crossover, edge);
        const auto hot = heat > 0.0f;

        // the strip it can be grabbed by
        if (hot)
        {
            g.setColour (theme().multibandCrossoverHighlight.withMultipliedAlpha (heat));
            g.fillRect (juce::Rectangle<float> (x - grabDistance, 0.0f, grabDistance * 2.0f, (float) getHeight()));
        }

        g.setColour (hot ? theme().multibandCrossoverHot : theme().multibandCrossover);
        g.drawLine (x, 0.0f, x, (float) getHeight(), hot ? 2.0f : 1.0f);

        const auto freq = crossovers[(size_t) edge]->get();
        const auto label = freq >= 1000.0f ? juce::String (freq / 1000.0f, 1) + "k" : juce::String (juce::roundToInt (freq));

        g.setColour (hot ? theme().multibandCrossoverTextHot : theme().multibandCrossoverText);
        g.drawText (label, juce::Rectangle<float> (x + 3.0f, (float) getHeight() - bottomMargin - fontHeight - 2.0f, 40.0f, fontHeight),
                    juce::Justification::centredLeft, false);
    }

    void selectBand (int band)
    {
        if (onBandSelected != nullptr)
            onBandSelected (layout.slots[(size_t) band]);
    }

    static void toggle (juce::AudioParameterBool* param)
    {
        param->beginChangeGesture();
        *param = !param->get();
        param->endChangeGesture();
    }

    static void resetToDefault (juce::AudioParameterFloat& param, const ParamIDs::ParameterInfo& info)
    {
        param.beginChangeGesture();
        param = info.defaultValue;
        param.endChangeGesture();
    }

    void timerCallback() override
    {
        // drained even while hidden, so the view doesn't open onto whatever was left in the queues
        auto drain = [] (AudioBufferQueue<float>& queue, SpectrumAnalyser& analyser)
        {
            std::array<float, 512> scratch;

            while (queue.getReadableSpace() > 0)
            {
                const auto popped = (size_t) queue.pop (scratch.data(), scratch.size());
                analyser.push (scratch.data(), popped);
            }
        };

        drain (collector.audioBufferQueueSpectrum, spectrum);
        drain (collector.audioBufferQueueInputSpectrum, inputSpectrum);

        if (!isShowing())
            return;

        spectrum.update();
        inputSpectrum.update();
        repaint();
    }

    ScopeDataCollector<float>& collector;
    // four times the scope's window: ~5Hz bins at 44.1k, so the lows either side of a low crossover separate
    SpectrumAnalyser spectrum { scope_constants::fftSize * 4 };
    SpectrumAnalyser inputSpectrum { scope_constants::fftSize * 4 }; // the same resolution, so the two lines compare

    // every slot's, and every crossover's, in their own order
    std::array<juce::AudioParameterFloat*, ParamIDs::numBands - 1> slotCrossovers {};
    std::array<juce::AudioParameterFloat*, ParamIDs::numBands> slotGains {};
    std::array<juce::AudioParameterBool*, ParamIDs::numBands> slotMutes {}, slotSolos {}, slotPowers {};

    // the same lined up band by band, and split by split, for the current layout
    std::array<juce::AudioParameterFloat*, ParamIDs::numBands - 1> crossovers {};
    std::array<juce::AudioParameterFloat*, ParamIDs::numBands> gains {};
    std::array<juce::AudioParameterBool*, ParamIDs::numBands> mutes {}, solos {}, powers {};

    MainRouting::BandLayout layout;
    juce::AudioProcessorValueTreeState& apvts;
    std::array<juce::AudioParameterChoice*, ParamIDs::numBands> slotTypes {};

    // each slot's in gain, and the same lined up band by band, for the reference line behind the spectrum
    std::array<juce::AudioParameterFloat*, ParamIDs::numBands> slotInGains {}, inGains {};
    float driveNorm = 0.0f; // the drive being dragged, normalised, accumulated here so small steps aren't lost to the parameter's rounding
    juce::AudioParameterInt* bandCount = nullptr; // what adding and removing bands changes

    float addX = 0.0f; // where the add preview sits while the mouse is on the top strip

    int numBands = 2;
    int activeBand = 0;
    int activeSlot = 0;

    Target hover, drag;
    // command held once the drag is under way for fine adjustments, a tenth of the usual movement
    static float dragScale (const juce::MouseEvent& e) { return e.mods.isCommandDown() ? 0.1f : 1.0f; }

    juce::Point<float> lastDragPosition;
    double dragLogFreq = 0.0;
    double slideOffset = 0.0, slideSpread = 0.0; // in log frequency, how far the band has moved and widened
    bool sliding = false;                        // past a click, so the splits' gestures are open

    // the closest two splits of one band get, in log frequency: about a quarter apart
    static constexpr double minSplitGap = 0.1;
    std::array<double, ParamIDs::numBands - 1> slideStartLogs {};
    float levelDelta = 0.0f;
    std::array<float, ParamIDs::numBands> dragStartGains {};
    bool exciter = false;

    static constexpr float fontHeight = 10.0f;
    static constexpr float thumbRadius = 4.0f;
    static constexpr float toggleSize = 14.0f;
    static constexpr float bottomMargin = 4.0f;  // under the drive readouts, off the box's rounded bottom
    static constexpr float driveWidth = 70.0f, driveDragPixels = 200.0f; // the readout's width, and the drag across its whole range
    static constexpr float driveRowHeight = fontHeight + 6.0f;          // the row along the bottom the readouts sit in, kept clear of the level lines
    static constexpr float addStripHeight = 22.0f, addBadgeSize = 12.0f; // the top strip that adds a band, and its plus
    static constexpr float toggleHitMargin = 3.0f;  // how far past its drawn edge a button still takes a click
    static constexpr float addClearance = 12.0f;    // the gap kept between the buttons and anywhere a click would add a band
    static constexpr float toggleGap = 2.0f;
    static constexpr float toggleInset = 6.0f; // from the band's corner
    static constexpr float grabDistance = 10.0f; // either side of a line, so the grabbable strip is twice this

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultibandView)
};
