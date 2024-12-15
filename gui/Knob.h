#pragma once

#include "../PluginProcessor.h"


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

        if (getParentComponent() != nullptr) { // on linux the parent happens to be broken somehow
            auto font = getParentComponent()->getLookAndFeel().getLabelFont(label);
            label.setFont(font);

        }

        label.setColour(juce::Label::textColourId, juce::Colours::white);
        label.setJustificationType(juce::Justification::centredTop);
        addAndMakeVisible(label);

        label.setText(kName, juce::dontSendNotification);
    }

    void paint(juce::Graphics &g) override
    {
        // auto bounds = getLocalBounds();
        // drawRotarySlider(g, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), knob.valueToProportionOfLength(knob.getValue()), 0.0f, 1.0f);
        auto outline = this->findColour(Slider::rotarySliderOutlineColourId);
        auto knobColour = this->findColour(Slider::rotarySliderFillColourId);

        auto bounds = juce::Rectangle<int>(knobBounds).toFloat().reduced(5.0f);

        g.setColour(outline);
        
        float size = std::min(knobBounds.getWidth(), knobBounds.getHeight());

        // the knob background
        // g.fillEllipse(juce::Rectangle<float>(size, size).reduced(5.0f).withCentre(bounds.getCentre()));

        // some circles or something
        g.setColour(knobColour);

        // g.drawEllipse(Rectangle<float>(size, size).reduced(7.0f).withCentre(bounds.getCentre()), 1.0f);
        g.drawEllipse(juce::Rectangle<float>(size, size).reduced(12.0f).withCentre(bounds.getCentre()), 2.0f);
        g.drawEllipse(juce::Rectangle<float>(size, size).reduced(20.0f).withCentre(bounds.getCentre()), 4.0f);
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