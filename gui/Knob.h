#pragma once

#include "../PluginProcessor.h"
#include "KnobUtils.h"
#include "GenericKnob.h"

class ParamKnob : public GenericKnob
{
public:
    ParamKnob(AudioPluginAudioProcessor &p, juce::String knobName, const ParamIDs::ParameterInfo& attachmentInfo, ParamUnits knobUnit = ParamUnits::none, ScopeContextType scopeContextType = ScopeContextType::LR_SCOPE) : 
    GenericKnob(p, knobName, attachmentInfo, knobUnit, scopeContextType) {
        
    }

    void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height, float sliderPos,
                        const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider &slider)
    {
        auto thumb = slider.findColour(juce::Slider::thumbColourId);

        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(5.0f);

        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto lineW = juce::jmin(8.0f, radius * 0.5f);
        auto arcRadius = radius - lineW * 0.5f;

        juce::Line<float> marker;

        float xOffset = std::sin(toAngle) * arcRadius;
        float yOffset = -std::cos(toAngle) * arcRadius;

        marker.setStart(xOffset * 0.8f + bounds.getCentreX(), yOffset * 0.8f + bounds.getCentreY());
        marker.setEnd(xOffset + bounds.getCentreX(), yOffset + bounds.getCentreY());

        juce::Path p;
        p.addLineSegment(marker, radius * 0.08f);
        g.setColour(thumb);
        g.strokePath(p, juce::PathStrokeType(radius * 0.08f, juce::PathStrokeType::JointStyle::curved, juce::PathStrokeType::EndCapStyle::rounded));
    }

    void paint(juce::Graphics &g) override
    {
        auto bounds = knobBounds.reduced(5.0f);
        
        auto outline = this->findColour(juce::Slider::rotarySliderOutlineColourId);
        auto knobColour = this->findColour(juce::Slider::rotarySliderFillColourId);


        g.setColour(outline);
        
        float size = std::min(knobBounds.getWidth(), knobBounds.getHeight());

        // the knob background
        // g.fillEllipse(juce::Rectangle<float>(size, size).reduced(5.0f).withCentre(bounds.getCentre()));

        // some circles or something
        g.setColour(knobColour);

        // g.drawEllipse(Rectangle<float>(size, size).reduced(7.0f).withCentre(bounds.getCentre()), 1.0f);
        g.drawEllipse(juce::Rectangle<float>(size, size).reduced(12.0f).withCentre(bounds.getCentre().toFloat()), 2.0f);
        g.drawEllipse(juce::Rectangle<float>(size, size).reduced(20.0f).withCentre(bounds.getCentre().toFloat()), 4.0f + dragAmount * 4.0f);

        auto rotary = getRotaryParameters();

        bounds.expand(5.0f, 5.0f);
        drawRotarySlider(g, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), valueToProportionOfLength(getValue()), rotary.startAngleRadians, rotary.endAngleRadians, *this);
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
        label.setBounds(bounds.removeFromBottom(18));

        knobBounds = bounds;
    }

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParamKnob)
};
