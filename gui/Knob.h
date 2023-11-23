#pragma once

#include "../PluginProcessor.h"
#include <JuceHeader.h>
#include "LookAndFeel/KnobLAF.h"

// enum of units, with strings attached to them
enum class ParamUnits {
    none,
    hz,
    ms,
    db,
    percent
};

// a map on all strings to their corresponding units
static std::map<ParamUnits, juce::String> unitStrings = {
    {ParamUnits::none, ""},
    {ParamUnits::hz, "Hz"},
    {ParamUnits::ms, "ms"},
    {ParamUnits::db, "dB"},
    {ParamUnits::percent, "%"}
};

class ParamKnob : public juce::Component
{
public:
    ParamKnob(AudioPluginAudioProcessor &p, juce::String name, juce::String attachmentID, ParamUnits unit = ParamUnits::none): processorRef(p) {
        knobAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.treeState, attachmentID, knob);
        jassert(knobAttachment);

        auto knobParamRange = p.treeState.getParameterRange(attachmentID);

        knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        knob.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        knob.setRange(knobParamRange.start, knobParamRange.end);

        knob.onDragStart = [this, unit] { 
            // display the parameter value as the text instead of the parameter name
            this->isDragging = true;
            this->label.setText(juce::String(this->knob.getValue(), 2) + unitStrings[unit], juce::dontSendNotification);
        };

        // when value is changing, set it to what the knob is, but only if we're dragging
        knob.onValueChange = [this, unit] {
            if (this->isDragging) {
                
                this->label.setText(juce::String(this->knob.getValue(), 2) + unitStrings[unit], juce::dontSendNotification);
            }
        };

        knob.onDragEnd = [this, name] { 
            // display the parameter name as the text again
            this->isDragging = false;
            this->label.setText(name, juce::dontSendNotification);
        };

        addAndMakeVisible(knob);
        
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        label.setJustificationType(juce::Justification::centredTop);
        label.setFont(KnobLAF::getTheFont(14.0f));
        addAndMakeVisible(label);

        label.setText(name, juce::dontSendNotification);
    }

    ~ParamKnob() override {
        knobAttachment = nullptr;
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        label.setBounds(bounds.removeFromBottom(20));
        knob.setBounds(bounds);
    }

    void paint(juce::Graphics &g) override
    {
        // g.setColour(juce::Colours::white);
        // g.drawRect(getLocalBounds(), 1);
    }

private:
    AudioPluginAudioProcessor &processorRef;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> knobAttachment = nullptr;

    Slider knob;
    Label label;

    bool isDragging = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParamKnob)
};