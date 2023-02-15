
#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "LookAndFeel/OtherLookAndFeel.h"

class ParameterKnob : public juce::Slider, private juce::Slider::Listener
{
public:
    ParameterKnob(AudioPluginAudioProcessor &p, juce::String knobIdParam, juce::String nameParam, float minRange, float maxRange) : processorRef(p)
    {
        knobValue = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.treeState, knobIdParam, knob);

        knob.setLookAndFeel(&pluginLookAndFeel);
        knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setRange(minRange, maxRange, 0.00001f);
        knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 90, 0);
        knob.setTextValueSuffix(" " + nameParam);
        addAndMakeVisible(&knob);
        knob.addListener(this);

        knobName = nameParam;
    }
    ~ParameterKnob() {
        knobValue = nullptr; // if this is not here, plugin will crash
    }

    void paint(juce::Graphics &g) override
    {      
        auto bounds = getLocalBounds().removeFromBottom(30);

        bounds.reduce(5, 2);

        // background to text
        g.setColour(juce::Colours::black);
        g.setOpacity(0.6f);
        // rounded rectangle
        g.fillRoundedRectangle(bounds.toFloat(), 7.0f);
        
        // text with gradient
        g.setGradientFill(juce::ColourGradient(juce::Colours::greenyellow, bounds.getWidth() / 2, bounds.getHeight() / 2, juce::Colours::cyan, bounds.getWidth(), bounds.getHeight(), false));
        g.setFont(15);
        g.drawFittedText(knobName, bounds, juce::Justification::centred, 1.5);
    }

    void resized() override
    {
        // scene.setBounds (0, 0, getWidth(), getHeight());

        auto bounds = getLocalBounds();
        bounds.removeFromBottom(30);

        knob.setBounds(bounds);
    }

    void sliderValueChanged(juce::Slider *slider) {
        juce::ignoreUnused(slider);
    };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> knobValue;

private:
    OtherLookAndFeel pluginLookAndFeel;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterKnob)

protected:
    AudioPluginAudioProcessor &processorRef;
    juce::Slider knob;
    juce::String knobName;
};
