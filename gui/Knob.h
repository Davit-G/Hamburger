#pragma once

#include "../PluginProcessor.h"
 
#include "LookAndFeel/KnobLAF.h"

// enum of units, with strings attached to them
enum class ParamUnits {
    none,
    hz,
    ms,
    db,
    percent,
    x
};

inline juce::String createParamString(float value, ParamUnits unit) noexcept {
    switch (unit) {
        case ParamUnits::hz:
            return juce::String(value, 1, false) + " Hz";
        case ParamUnits::ms:
            return juce::String(value, 1, false) + " ms";
        case ParamUnits::db:
            return juce::String(value, 1, false) + " dB";
        case ParamUnits::percent:
            return juce::String(value, 2, false) + " %";
        case ParamUnits::x:
            return juce::String(value, 0, false) + "x";
        default:
            return juce::String(value, 2, false);
    }
}

class ParamKnob : public juce::Component
{
public:
    ParamKnob(AudioPluginAudioProcessor &p, juce::String knobName, juce::ParameterID attachmentID, ParamUnits knobUnit = ParamUnits::none): processorRef(p), kName(knobName), unit(knobUnit) {
        knobAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.treeState, attachmentID.getParamID(), knob);
        jassert(knobAttachment);

        auto knobParamRange = p.treeState.getParameterRange(attachmentID.getParamID());

        knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        knob.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        
        if (knobUnit == ParamUnits::x) {
            knob.setRange(knobParamRange.start, knobParamRange.end, 1.0);
        } else {
            knob.setRange(knobParamRange.start, knobParamRange.end, 0.001);
        }

        knob.setPaintingIsUnclipped(true);

        setPaintingIsUnclipped(true);
        setBufferedToImage(true);

        setName(knobName);

        knob.onDragStart = [this] { 
            // display the parameter value as the text instead of the parameter name
            this->isDragging = true;
            this->label.setText(createParamString(this->knob.getValue(), this->unit), juce::dontSendNotification);
        };

        // when value is changing, set it to what the knob is, but only if we're dragging
        knob.onValueChange = [this] {
            if (this->isDragging) {
                this->label.setText(createParamString(this->knob.getValue(), this->unit), juce::dontSendNotification);
            } else {
                this->label.setText(this->kName, juce::dontSendNotification);
            }
        };

        knob.onDragEnd = [this] { 
            // display the parameter name as the text again
            this->isDragging = false;
            this->label.setText(this->kName, juce::dontSendNotification);
        };

        addAndMakeVisible(knob);

        label.setColour(juce::Label::textColourId, juce::Colours::white);
        label.setJustificationType(juce::Justification::centredTop);
        label.setFont(KnobLAF::getTheFont(12.0f));
        addAndMakeVisible(label);

        label.setText(kName, juce::dontSendNotification);
    }

    void drawRotaryBG(juce::Graphics &g, int x, int y, int width, int height, float sliderPos,
                          const float rotaryStartAngle, const float rotaryEndAngle)
    {
        auto outline = knob.findColour(Slider::rotarySliderOutlineColourId);
        auto fill = knob.findColour(Slider::rotarySliderFillColourId);
        auto thumb = knob.findColour(Slider::thumbColourId);
        auto knobColour = knob.findColour(Slider::rotarySliderFillColourId);

        auto bounds = Rectangle<int>(x, y, width, height).toFloat().reduced(5.0f);

        auto radius = jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto lineW = jmin(8.0f, radius * 0.5f);
        auto arcRadius = radius - lineW * 0.5f;

        // g.setColour(outline);
        
        auto size = std::min(width, height);

        // the knob background
        // g.fillEllipse(Rectangle<float>(size, size).reduced(5.0f).withCentre(bounds.getCentre()));

        // some circles or something
        // g.setColour(knobColour);

        // g.drawEllipse(Rectangle<float>(size, size).reduced(7.0f).withCentre(bounds.getCentre()), 1.0f);
        // g.drawEllipse(Rectangle<float>(size, size).reduced(12.0f).withCentre(bounds.getCentre()), 2.0f);
        // g.drawEllipse(Rectangle<float>(size, size).reduced(20.0f).withCentre(bounds.getCentre()), 4.0f);   
        // the marker for where the knob is
        // capsule shaped, so maybe thick line going outwards, with rounded caps at the end
        Line<float> marker;

        float xOffset = std::sin(toAngle) * arcRadius;
        float yOffset = -std::cos(toAngle) * arcRadius;

        marker.setStart(xOffset * 0.8f + bounds.getCentreX(), yOffset * 0.8f + bounds.getCentreY());
        marker.setEnd(xOffset + bounds.getCentreX(), yOffset + bounds.getCentreY());
        
        Path p;
        p.addLineSegment(marker, radius * 0.08f);
        g.setColour(thumb);
        g.strokePath(p, PathStrokeType(radius * 0.08f, PathStrokeType::JointStyle::curved, PathStrokeType::EndCapStyle::rounded));
        
    }

    void paint(juce::Graphics &g) override
    {
        // auto bounds = getLocalBounds();
        // drawRotarySlider(g, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), knob.valueToProportionOfLength(knob.getValue()), 0.0f, 1.0f);
        auto outline = knob.findColour(Slider::rotarySliderOutlineColourId);
        auto knobColour = knob.findColour(Slider::rotarySliderFillColourId);

        auto bounds = Rectangle<int>(knobBounds).toFloat().reduced(5.0f);

        g.setColour(outline);
        
        float size = std::min(knobBounds.getWidth(), knobBounds.getHeight());

        // the knob background
        g.fillEllipse(Rectangle<float>(size, size).reduced(5.0f).withCentre(bounds.getCentre()));

        // some circles or something
        g.setColour(knobColour);

        // g.drawEllipse(Rectangle<float>(size, size).reduced(7.0f).withCentre(bounds.getCentre()), 1.0f);
        g.drawEllipse(Rectangle<float>(size, size).reduced(12.0f).withCentre(bounds.getCentre()), 2.0f);
        g.drawEllipse(Rectangle<float>(size, size).reduced(20.0f).withCentre(bounds.getCentre()), 4.0f);
    }

    ~ParamKnob() override {
        knobAttachment = nullptr;
    }

    void resized() override
    {
        auto amt = knob.valueToProportionOfLength(knob.getValue());

        auto bounds = getLocalBounds();
        label.setBounds(bounds.removeFromBottom(18));
        knob.setBounds(bounds);

        knobBounds = bounds;
    }

private:
    AudioPluginAudioProcessor &processorRef;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> knobAttachment = nullptr;

    Slider knob;
    Label label;

    juce::Rectangle<int> knobBounds;

    juce::String kName;
    ParamUnits unit;

    bool isDragging = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParamKnob)
};