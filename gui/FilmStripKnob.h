
#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "LookAndFeel/SaturationLookAndFeel.h"

#include "ParameterKnob.h"

class FilmStripKnob : public ParameterKnob
{
public:
    FilmStripKnob(AudioPluginAudioProcessor &p, juce::String knobIdParam, juce::String nameParam, float minRange, float maxRange, juce::Image image, int knobSize) : processorRef(p), ParameterKnob(p, knobIdParam, nameParam, minRange, maxRange)
    , pluginLookAndFeel(knobSize, image)
    {
        juce::ignoreUnused(p);

        knobValue = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.treeState, knobIdParam, knob);

        knob.setLookAndFeel(&pluginLookAndFeel);
        knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setRange(minRange, maxRange, 0.00001f);
        knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 90, 0);
        addAndMakeVisible(&knob);
        
        this->knobSize = knobSize;

        knobName = nameParam;
    };

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

    
private:
    SaturationLookAndFeel pluginLookAndFeel;
    AudioPluginAudioProcessor &processorRef;


    int knobSize;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilmStripKnob)
};
