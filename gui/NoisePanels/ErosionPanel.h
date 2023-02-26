#pragma once

#include <Juceheader.h>
#include "../../PluginProcessor.h"

class ErosionPanel : public juce::Component
{
public:
    ErosionPanel(AudioPluginAudioProcessor &p) : processorRef(p)
    {

        juce::Image smallKnobImage = juce::ImageCache::getFromMemory(BinaryData::ingain2_min_png, BinaryData::ingain2_min_pngSize);
        juce::Image knobImage = juce::ImageCache::getFromMemory(BinaryData::knob_min_png, BinaryData::knob_min_pngSize);
        juce::Image sizzleImage = juce::ImageCache::getFromMemory(BinaryData::sizzleknob_min_png, BinaryData::sizzleknob_min_pngSize);

        noiseAmount = std::make_unique<FilmStripKnob>(p, "noiseAmount", "Amount", 0.0f, 100.0f, sizzleImage, 72);
        addAndMakeVisible(noiseAmount.get());

        noiseFrequency = std::make_unique<FilmStripKnob>(p, "noiseFrequency", "Frequency", 20.0f, 20000.f, knobImage, 54);
        addAndMakeVisible(noiseFrequency.get());

        noiseQ = std::make_unique<FilmStripKnob>(p, "noiseQ", "Q", 0.1f, 3.0f, knobImage, 54);
        addAndMakeVisible(noiseQ.get());
    }

    ~ErosionPanel() {}

    void resized() override
    {
        float center = getWidth() * 0.5;
        noiseAmount->setBounds(center - noiseAmount->getKnobSize() * 0.5, 0, noiseAmount->getKnobSize(), noiseAmount->getKnobHeight());
        noiseFrequency->setBounds(center - noiseFrequency->getKnobSize() * 0.5 - 85, 5, noiseFrequency->getKnobSize(), noiseFrequency->getKnobHeight());
        noiseQ->setBounds(center - noiseQ->getKnobSize() * 0.5 + 85, 0, noiseQ->getKnobSize(), noiseQ->getKnobHeight());
    }

   

private:
    AudioPluginAudioProcessor &processorRef;

    std::unique_ptr<FilmStripKnob> noiseAmount = nullptr;
    std::unique_ptr<FilmStripKnob> noiseFrequency = nullptr;
    std::unique_ptr<FilmStripKnob> noiseQ = nullptr;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ErosionPanel);
};