#pragma once

#include "../PluginProcessor.h"
#include "../utils/KnobUtils.h"
#include "GenericKnob.h"

class ParamKnob : public GenericKnob
{
public:
    static constexpr int labelHeight = 18;

    // fixed parameter
    ParamKnob(AudioPluginAudioProcessor &p, juce::String knobName, const ParamIDs::ParameterInfo& attachmentInfo, ScopeContextType scopeContextType = ScopeContextType::LR_SCOPE) : 
    GenericKnob(p, knobName, attachmentInfo, scopeContextType) {}

    // slot macro
    ParamKnob(AudioPluginAudioProcessor &p, juce::String knobName, SlotId slot, const ParamIDs::ParameterInfo& attachmentInfo, ScopeContextType scopeContextType = ScopeContextType::LR_SCOPE) : 
    GenericKnob(p, knobName, slot, attachmentInfo, scopeContextType) {}

    // thickness as a share of the knob's radius
    void drawThumb(juce::Graphics &g, juce::Rectangle<float> bounds, float sliderPos, juce::Colour colour, float thickness)
    {
        bounds = bounds.reduced(5.0f);

        auto rotary = getRotaryParameters();
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotary.startAngleRadians + sliderPos * (rotary.endAngleRadians - rotary.startAngleRadians);
        auto lineW = juce::jmin(8.0f, radius * 0.5f);
        auto arcRadius = radius - lineW * 0.5f;

        juce::Line<float> marker;

        float xOffset = std::sin(toAngle) * arcRadius;
        float yOffset = -std::cos(toAngle) * arcRadius;

        marker.setStart(xOffset * 0.8f + bounds.getCentreX(), yOffset * 0.8f + bounds.getCentreY());
        marker.setEnd(xOffset + bounds.getCentreX(), yOffset + bounds.getCentreY());

        juce::Path p;
        p.addLineSegment(marker, radius * thickness);
        g.setColour(colour);
        g.strokePath(p, juce::PathStrokeType(radius * thickness, juce::PathStrokeType::JointStyle::curved, juce::PathStrokeType::EndCapStyle::rounded));
    }

    void paint(juce::Graphics &g) override
    {
        auto bounds = knobBounds.reduced(5.0f);

        float size = std::min(knobBounds.getWidth(), knobBounds.getHeight());
        const auto centre = bounds.getCentre().toFloat();
        const auto arcs = modArcs();

        // as many of the modulations as have room for a ring, one inside the next from just inside the outer ring
        auto rings = 0;

        while (rings < (int) arcs.size() && ringRadius(size, rings) >= 3.0f)
            ++rings;

        // two rings, the inner one thickening while it's dragged, and moved in past the modulations' rings
        const auto innerRadius = arcs.empty() ? size * 0.5f - 20.0f : ringRadius(size, rings);

        g.setColour(colours().main.interpolatedWith(colours().mainHeld, dragAmount));
        g.drawEllipse(juce::Rectangle<float>(size, size).reduced(12.0f).withCentre(centre), 2.0f);

        if (innerRadius >= 3.0f)
            g.drawEllipse(juce::Rectangle<float>(innerRadius * 2.0f, innerRadius * 2.0f).withCentre(centre), 4.0f + dragAmount * 4.0f);

        drawModulation(g, size, centre, arcs, rings);

        bounds.expand(5.0f, 5.0f);
        drawThumb(g, bounds.toFloat(), (float) valueToProportionOfLength(getValue()), theme().knobThumb, 0.08f);

        // a thinner one where the audio really is, when global drive or the like scales it
        if (auto scaled = scaledProportion())
            drawThumb(g, bounds.toFloat(), *scaled, theme().scaledMarker, 0.04f);
    }

    static constexpr float ringWidth = 4.0f, ringGap = 1.5f;

    // the centre of a modulation's ring, the first one a gap inside the outer ring
    static float ringRadius(float size, int ring) { return size * 0.5f - 13.0f - ringGap - ringWidth * 0.5f - (ringWidth + ringGap) * (float) ring; }

    /*  A ring of its own for each modulation routed here, under the thumb: faintly the whole way round, how far it can
        reach in the modulation colour, and in its source's colour, an arc and a dot from where the knob is to where it's
        being moved right now */
    void drawModulation(juce::Graphics& g, float size, juce::Point<float> centre, const std::vector<ModArc>& arcs, int rings)
    {
        const auto rotary = getRotaryParameters();
        const auto angle = [&](float proportion) { return rotary.startAngleRadians + proportion * (rotary.endAngleRadians - rotary.startAngleRadians); };

        for (int ring = 0; ring < rings; ++ring)
        {
            const auto& arc = arcs[(size_t) ring];
            const auto radius = ringRadius(size, ring);

            juce::Path track, reach, moved;
            track.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, rotary.startAngleRadians, rotary.endAngleRadians, true);
            reach.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, angle(arc.from), angle(arc.to), true);
            moved.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, angle(arc.base), angle(arc.at), true);

            g.setColour(arc.colour.withMultipliedAlpha(0.15f));
            g.strokePath(track, juce::PathStrokeType(ringWidth));

            g.setColour(theme().modulationHighlight.withMultipliedAlpha(0.45f));
            g.strokePath(reach, juce::PathStrokeType(ringWidth));

            g.setColour(arc.colour);
            g.strokePath(moved, juce::PathStrokeType(ringWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.fillEllipse(juce::Rectangle<float>(ringWidth + 1.0f, ringWidth + 1.0f).withCentre(centre.getPointOnCircumference(radius, angle(arc.at))));
        }
    }

    void timerCallback() override
    {
        if (isDragging) // hold at full brightness until the user lets go
            return;

        dragAmount *= dragDecayRate;

        if (dragAmount < dragDecayThreshold) {
            dragAmount = 0.0f;
            stopTimer();
        }

        repaint();
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        label.setBounds(bounds.removeFromBottom(labelHeight));

        knobBounds = bounds;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParamKnob)
};
