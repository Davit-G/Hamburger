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
    GenericKnob(p, knobName, attachmentInfo, scopeContextType) {
        
    }

    // slot macro
    ParamKnob(AudioPluginAudioProcessor &p, juce::String knobName, SlotId slot, const ParamIDs::ParameterInfo& attachmentInfo, ScopeContextType scopeContextType = ScopeContextType::LR_SCOPE) : 
    GenericKnob(p, knobName, slot, attachmentInfo, scopeContextType) {

    }

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

        // the knob background
        // g.fillEllipse(juce::Rectangle<float>(size, size).reduced(5.0f).withCentre(bounds.getCentre()));

        // some circles or something
        g.setColour(colours().main.interpolatedWith(colours().mainHeld, dragAmount));

        // g.drawEllipse(Rectangle<float>(size, size).reduced(7.0f).withCentre(bounds.getCentre()), 1.0f);
        g.drawEllipse(juce::Rectangle<float>(size, size).reduced(12.0f).withCentre(bounds.getCentre().toFloat()), 2.0f);
        g.drawEllipse(juce::Rectangle<float>(size, size).reduced(20.0f).withCentre(bounds.getCentre().toFloat()), 4.0f + dragAmount * 4.0f);

        bounds.expand(5.0f, 5.0f);
        drawThumb(g, bounds.toFloat(), (float) valueToProportionOfLength(getValue()), theme().knobThumb, 0.08f);

        // a thinner one where the audio really is, when global drive or the like scales it
        if (auto scaled = scaledProportion())
            drawThumb(g, bounds.toFloat(), *scaled, theme().scaledMarker, 0.04f);
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
        auto amt = valueToProportionOfLength(getValue());

        auto bounds = getLocalBounds();
        label.setBounds(bounds.removeFromBottom(labelHeight));

        knobBounds = bounds;
    }

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParamKnob)
};
