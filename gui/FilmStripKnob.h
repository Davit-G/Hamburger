
#pragma once

#include <JuceHeader.h>

#include "../PluginProcessor.h"
#include "./LookAndFeel/SaturationLookAndFeel.h"

// #include "ParameterKnob.h"

class FilmStripKnob : public juce::Slider, private juce::Slider::Listener
{
public:
    FilmStripKnob(AudioPluginAudioProcessor &p, juce::String knobIdParam, juce::String nameParam, float minRange, float maxRange, juce::Image image, int knobSize) : processorRef(p), pluginLookAndFeel(knobSize, image)
    {
        knobAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.treeState, knobIdParam, knob);

        knob.setLookAndFeel(&pluginLookAndFeel);
        knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setRange(minRange, maxRange, 0.00001f);
        knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 90, 0);
        knob.setTextValueSuffix(" " + nameParam);
        addAndMakeVisible(&knob);

        // addListener(this);
        
        this->knobSize = knobSize;
        knobName = nameParam;
    };

    ~FilmStripKnob() {
        knobAttachment = nullptr; // if this is not here, plugin will crash
    }

    void paint(juce::Graphics &g) override
    {    
        juce::ignoreUnused(g);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        knob.setBounds(bounds);
    }

    int getKnobSize() {
        return knobSize;
    }

    void sliderValueChanged(juce::Slider *slider) {
        juce::ignoreUnused(slider);
    };
    
private:
    AudioPluginAudioProcessor &processorRef;
    SaturationLookAndFeel pluginLookAndFeel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> knobAttachment;

    int knobSize;
    juce::String knobName;
    juce::Slider knob;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilmStripKnob)
};
