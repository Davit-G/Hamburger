#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"
#include "../../../dsp/Distortions/waveshape/Waveshape.h"
#include "../../RectSlider.h"

// a shape group's colour, shared by the pad's dots and the legend beside it
inline juce::Colour waveshapeGroupColour (waveshapes::Group group)
{
    switch (group)
    {
        case waveshapes::Analog:  return theme().waveshapeAnalog;
        case waveshapes::Digital: return theme().waveshapeDigital;
        case waveshapes::Folding: return theme().waveshapeFolding;
        case waveshapes::Heavy:   return theme().waveshapeHeavy;
        case waveshapes::GroupCount:
        default:                  return juce::Colours::white;
    }
}

inline juce::Colour waveshapeIdleColour (waveshapes::Group group)
{
    return waveshapeGroupColour (group).interpolatedWith (juce::Colours::black, 0.3f);
}

class WaveshapePad : public juce::Component,
                    public juce::SettableTooltipClient,
                    private juce::Timer
{
public:
    WaveshapePad (AudioPluginAudioProcessor& p, SlotId slot)
        : scopeContext (p.getScopeContext()),
          xParameter (*p.treeState.getParameter (paramIdFor (slot, ParamIDs::waveshapeX).getParamID())),
          yParameter (*p.treeState.getParameter (paramIdFor (slot, ParamIDs::waveshapeY).getParamID())),
          xAttachment (xParameter, [this] (float value) { x = value; handleMoved(); }),
          yAttachment (yParameter, [this] (float value) { y = value; handleMoved(); })
    {
        xAttachment.sendInitialUpdate();
        yAttachment.sendInitialUpdate();

        // the disc and dots only move with the zoom, so they're drawn into an image and only the handle is drawn per repaint
        map.setBufferedToImage (true);
        map.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (map);

        // plain text can't show a swatch, so the tip names the colours the panel's legend shows
        setTooltip ("WAVESHAPE: similar shapes sit together. Drag, command for finer moves");
    }

    // a bit per waveshapes::Group with a shape in the blend at the handle
    unsigned getActiveGroups() const noexcept { return activeGroups; }

    // when the handle moves into or out of a group's shapes
    std::function<void()> onActiveGroupsChanged;

    static bool isActive (unsigned groups, waveshapes::Group group) { return (groups & (1u << group)) != 0; }

    void resized() override { map.setBounds (getLocalBounds()); }

    // over the cached map, so moving the handle doesn't redraw 260 dots
    void paintOverChildren (juce::Graphics& g) override
    {
        const auto handleOnScreen = toScreen (x, y);

        for (int i = 0; i < blend.count; ++i)
        {
            const auto weight = (float) blend.weight[i];

            if (weight <= audibleWeight)
                continue;

            const auto& point = pointForShape (blend.index[i]);

            g.setColour (theme().waveshapeBlendLine.withMultipliedAlpha (0.35f + 0.45f * weight));
            g.drawLine ({ handleOnScreen, toScreen (point.x, point.y) }, 0.5f + 3.0f * weight);
        }

        g.setColour (theme().waveshapeHandle);
        g.drawEllipse (juce::Rectangle<float> (handleRadius * 2.0f, handleRadius * 2.0f).withCentre (handleOnScreen), 2.0f);
    }

    void mouseDown (const juce::MouseEvent& event) override
    {
        scopeContext.setType (ScopeContextType::WAVESHAPE);
        xAttachment.beginGesture();
        yAttachment.beginGesture();

        clickTarget = fromScreen (event.position);
        handle = { x, y };
        lastMouse = event.position;
        dragging = false;
    }

    void mouseDrag (const juce::MouseEvent& event) override
    {
        const auto moved = event.position - lastMouse;
        lastMouse = event.position;

        if (! dragging && event.getDistanceFromDragStart() < 2)
            return;

        if (! dragging)
        {
            dragging = true;
            zoomTo (dragZoom);
        }

        const auto area = padArea();
        const auto scale = event.mods.isCommandDown() ? fineDragScale : dragScale;

        handle += juce::Point<float> (moved.x / area.getWidth(), -moved.y / area.getHeight()) * scale;
        moveHandle();
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (! dragging)
        {
            handle = clickTarget;
            moveHandle();
        }

        dragging = false;
        zoomTo (restZoom);

        xAttachment.endGesture();
        yAttachment.endGesture();
        scopeContext.startDecaying();
    }

private:
    static constexpr float dotRadius = 2.5f, handleRadius = 6.0f;
    static constexpr float dragScale = 0.3f, fineDragScale = 0.05f;
    static constexpr float restZoom = 2.5f, dragZoom = 10.0f;
    static constexpr double audibleWeight = 0.02;

    // inset so dots and the handle at the edges of the map stay whole
    juce::Rectangle<float> padArea() const { return getLocalBounds().toFloat().reduced (handleRadius + 1.0f); }

    float fisheyeD() const { return zoom * 2.0f - 1.0f; }

    juce::Point<float> toScreen (double mapX, double mapY) const
    {
        const auto area = padArea();
        const auto centre = area.getCentre();
        const auto offset = juce::Point<float> ((float) (mapX - x) * area.getWidth(), (float) (y - mapY) * area.getHeight());
        const auto distance = offset.getDistanceFromOrigin();

        if (distance <= 0.0f)
            return centre;

        const auto radius = area.getWidth() * 0.5f;
        const auto share = distance / (radius * 2.0f);
        const auto d = fisheyeD();
        const auto warped = (d + 1.0f) * share / (d * share + 1.0f);

        return centre + offset * (warped * radius / distance);
    }

    // the other way, for a click: g inverted is u = g / (d + 1 - d g)
    juce::Point<float> fromScreen (juce::Point<float> position) const
    {
        const auto area = padArea();
        const auto offset = position - area.getCentre();
        const auto distance = offset.getDistanceFromOrigin();

        if (distance <= 0.0f)
            return { x, y };

        const auto radius = area.getWidth() * 0.5f;
        const auto d = fisheyeD();
        const auto warped = juce::jmin (distance / radius, 0.999f);
        const auto share = warped / (d + 1.0f - d * warped);
        const auto mapOffset = offset * (share * radius * 2.0f / distance);

        return { x + mapOffset.x / area.getWidth(), y - mapOffset.y / area.getHeight() };
    }

    // every dot moves with the handle, so the cached map has to be redrawn along with it
    void handleMoved()
    {
        updateBlend();
        map.repaint();
        repaint();
    }

    // eased there rather than jumped, so the map visibly grows and settles around the handle
    void zoomTo (float newZoom)
    {
        targetZoom = newZoom;
        startTimerHz (60);
    }

    void timerCallback() override
    {
        zoom += (targetZoom - zoom) * 0.25f;

        if (std::abs (targetZoom - zoom) < 0.01f)
        {
            zoom = targetZoom;
            stopTimer();
        }

        map.repaint();
        repaint();
    }

    // the map's point for a shape's table index
    static const waveshapes::MapPoint& pointForShape (int shapeIndex)
    {
        for (const auto& point : waveshapes::mapPoints)
            if (point.index == shapeIndex)
                return point;

        jassertfalse;
        return waveshapes::mapPoints[0];
    }

    // the part of the pad that only changes with the zoom: the disc backing and a dot per shape, buffered to an image
    struct MapLayer : public juce::Component
    {
        explicit MapLayer (WaveshapePad& padToDraw) : pad (padToDraw) {}

        void paint (juce::Graphics& g) override
        {
            const auto area = pad.padArea().expanded (handleRadius);

            juce::Path backing;
            backing.addEllipse (area);

            g.setColour (theme().waveshapeRingFill);
            g.fillPath (backing);
            g.setColour (theme().waveshapeRingOutline);
            g.strokePath (backing, juce::PathStrokeType (1.5f));

            for (const auto& point : waveshapes::mapPoints)
            {
                g.setColour (waveshapeGroupColour (waveshapes::shapes[(size_t) point.index].group));
                g.fillEllipse (juce::Rectangle<float> (dotRadius * 2.0f, dotRadius * 2.0f).withCentre (pad.toScreen (point.x, point.y)));
            }
        }

        WaveshapePad& pad;
    };

    // the blend the DSP is running at the handle, from the same lookup it uses: for the lines, and the legend's groups
    void updateBlend()
    {
        blend = waveshapes::mixAt (waveshapes::mapPoints, (int) std::size (waveshapes::mapPoints), x, y);

        auto groups = 0u;

        for (int i = 0; i < blend.count; ++i)
            if (blend.weight[i] > audibleWeight)
                groups |= 1u << waveshapes::shapes[(size_t) blend.index[i]].group;

        if (groups == activeGroups)
            return;

        activeGroups = groups;

        if (onActiveGroupsChanged != nullptr)
            onActiveGroupsChanged();
    }

    // the nearest point on the disc the pad draws, radius 0.5 around the middle of the map
    static juce::Point<float> insideDisc (juce::Point<float> point)
    {
        const juce::Point<float> centre { 0.5f, 0.5f };
        const auto offset = point - centre;
        const auto distance = offset.getDistanceFromOrigin();
        return distance > 0.5f ? centre + offset * (0.5f / distance) : point;
    }

    void moveHandle()
    {
        handle = insideDisc (handle);
        xAttachment.setValueAsPartOfGesture (handle.x);
        yAttachment.setValueAsPartOfGesture (handle.y);
    }

    MapLayer map { *this };

    ScopeContext& scopeContext;
    juce::RangedAudioParameter& xParameter;
    juce::RangedAudioParameter& yParameter;
    juce::ParameterAttachment xAttachment, yAttachment;
    float x = 0.5f, y = 0.5f;          // where the parameters are, as the host last reported them
    waveshapes::Mix blend;
    unsigned activeGroups = ~0u;
    juce::Point<float> handle;           // the drag in progress, in map units
    juce::Point<float> lastMouse;        // on the component

    float zoom = restZoom, targetZoom = restZoom;
    bool dragging = false;               // past a click's worth of movement
    juce::Point<float> clickTarget;      // where letting go without dragging jumps to

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveshapePad)
};

class WaveshapePanel : public Panel,
                       public juce::TooltipClient
{
public:
    WaveshapePanel (AudioPluginAudioProcessor& p, SlotId slot = SlotId { ModuleId::main, 0 })
        : Panel (p, "WAVESHAPE", &Theme::waveshape, slot),
          pad (p, slot),
          drive (p, "DRIVE", slot, ParamIDs::waveshapeDrive, ScopeContextType::WAVESHAPE),
          smooth (p, "SMOOTH", slot, ParamIDs::waveshapeSmooth, ScopeContextType::WAVESHAPE),
          bias (p, "BIAS", slot, ParamIDs::waveshapeBias, ScopeContextType::WAVESHAPE),
          asym (p, "ASYM", slot, ParamIDs::waveshapeAsym, ScopeContextType::WAVESHAPE)
    {
        addAndMakeVisible (pad);
        pad.onActiveGroupsChanged = [this] { repaint(); };

        for (auto* slider : { &drive, &smooth, &bias, &asym })
            addAndMakeVisible (slider);
    }

    void paint (juce::Graphics& g) override
    {
        g.setFont (legendFont());

        const auto active = pad.getActiveGroups();

        for (int index = 0; index < waveshapes::GroupCount; ++index)
        {
            const auto group = (waveshapes::Group) index;
            const auto pill = pillArea (group);

            g.setColour (WaveshapePad::isActive (active, group) ? waveshapeGroupColour (group) : waveshapeIdleColour (group));
            g.fillRoundedRectangle (pill, rowHeight * 0.5f);

            g.setColour (theme().waveshapePillText);
            g.drawText (shortNames[index], pill, juce::Justification::centred, false);
        }
    }

    juce::String getTooltip() override
    {
        static constexpr const char* descriptions[waveshapes::GroupCount] {
            "Analog / asymmetric / tube",
            "Digital / bit reduction",
            "Foldback / west coast",
            "Heavy / fuzz / noise"
        };

        const auto group = pillAt (getMouseXYRelative().toFloat());

        return group < 0 ? juce::String() : juce::String (descriptions[group]);
    }

    void resized() override
    {
        layoutSaturation(pad, noIcon, { &drive, &smooth, &bias, &asym });

        // the pad stays square either way, since the map is one. compact, it keeps clear of the label space like a knob would
        const auto area = usesCompactLayout() ? dialAreaOf(pad.getBounds()) : pad.getBounds();
        const auto side = juce::jmin(area.getWidth(), area.getHeight());
        pad.setBounds(area.withSizeKeepingCentre(side, side));

        legendInRows = false;

        for (int group = 0; group < waveshapes::GroupCount && ! legendInRows; ++group)
            for (auto* slider : { &drive, &smooth, &bias, &asym })
                if (pillArea ((waveshapes::Group) group).toNearestInt().intersects (slider->getBounds()))
                    legendInRows = true;
    }

private:
    static constexpr const char* shortNames[waveshapes::GroupCount] { "ANALOG", "DIGITAL", "FOLDING", "HEAVY" };
    static constexpr float rowHeight = 13.0f, rowGap = 2.0f, pillPadding = 5.0f, padGap = 6.0f, cornerInset = 6.0f;

    bool legendInRows = false; // decided in resized, by whether the columns beside the pad have room

    juce::Font legendFont() { return getLookAndFeel().getPopupMenuFont().withHeight (rowHeight * 0.75f); }

    float pillWidth (int group) { return juce::GlyphArrangement::getStringWidth (legendFont(), shortNames[group]) + pillPadding * 2.0f; }

    juce::Rectangle<float> pillArea (waveshapes::Group group)
    {
        const auto onLeft = group == waveshapes::Analog || group == waveshapes::Digital;
        const auto second = group % 2 == 1;
        const auto width = pillWidth (group);

        if (legendInRows)
        {
            const auto corner = getLocalBounds().toFloat().reduced (cornerInset);
            const auto partner = pillWidth (onLeft ? (second ? waveshapes::Analog : waveshapes::Digital)
                                                    : (second ? waveshapes::Folding : waveshapes::Heavy));

            const auto x = onLeft ? corner.getX() + (second ? partner + rowGap : 0.0f)
                                  : corner.getRight() - (second ? width : width + rowGap + partner);

            return { x, corner.getY(), width, rowHeight };
        }

        const auto padBounds = pad.getBounds().toFloat();
        const auto row = second ? 1.0f : 0.0f;

        return { onLeft ? padBounds.getX() - padGap - width : padBounds.getRight() + padGap,
                 padBounds.getY() + row * (rowHeight + rowGap),
                 width, rowHeight };
    }

    int pillAt (juce::Point<float> position)
    {
        for (int group = 0; group < waveshapes::GroupCount; ++group)
            if (pillArea ((waveshapes::Group) group).expanded (2.0f).contains (position))
                return group;

        return -1;
    }

    WaveshapePad pad;
    juce::Component noIcon;

    RectSlider drive;
    RectSlider smooth;
    RectSlider bias;
    RectSlider asym;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveshapePanel)
};
