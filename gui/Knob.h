#pragma once

#include "../PluginProcessor.h"
#include <JuceHeader.h>
#include "LookAndFeel/KnobLAF.h"

class ParamKnob : public juce::Component
{
public:
    ParamKnob(AudioPluginAudioProcessor &p, juce::String name, juce::String attachmentID, float minRange, float maxRange): processorRef(p) {
        knobAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.treeState, attachmentID, knob);
        jassert(knobAttachment);

        knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        knob.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        knob.setRange(minRange, maxRange); 
        addAndMakeVisible(knob);
        
        label.setText(name, juce::dontSendNotification);
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        label.setJustificationType(juce::Justification::centredTop);
        label.setFont(KnobLAF::getTheFont(14.0f));
        addAndMakeVisible(label);

        
    }

    ~ParamKnob() {
        knobAttachment = nullptr;
    }

    void resized() override
    {
        
        // auto bounds = getLocalBounds();
        // label.setBounds(bounds.removeFromBottom(bounds.proportionOfHeight(0.25f)));
        // knob.setBounds(bounds);

        auto bounds = getLocalBounds();
        label.setBounds(bounds.removeFromBottom(20));
        knob.setBounds(bounds);
    }

    void paint(juce::Graphics &g) override
    {
        // g.setColour(juce::Colours::black);
        // g.drawRect(getLocalBounds(), 1);
    }

private:
    AudioPluginAudioProcessor &processorRef;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> knobAttachment;

    Slider knob;
    Label label;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParamKnob)
};