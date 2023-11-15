#pragma once

#include <Juceheader.h>
#include "../../PluginProcessor.h"

class ReduxPanel : public juce::Component
{
public:
    ReduxPanel(AudioPluginAudioProcessor &p) : processorRef(p)
    {

        juce::Image smallKnobImage = juce::ImageCache::getFromMemory(BinaryData::ingain2_min_png, BinaryData::ingain2_min_pngSize);
        juce::Image knobImage = juce::ImageCache::getFromMemory(BinaryData::knob_min_png, BinaryData::knob_min_pngSize);
        juce::Image sizzleImage = juce::ImageCache::getFromMemory(BinaryData::sizzleknob_min_png, BinaryData::sizzleknob_min_pngSize);

        downsampleAmount = std::make_unique<FilmStripKnob>(p, "downsampleAmount", "Downsample", 0.f, 20.f, knobImage, 54);
        addAndMakeVisible(downsampleAmount.get());

        bitReduction = std::make_unique<FilmStripKnob>(p, "bitReduction", "Bit Reduce", 1.0f, 32.0f, knobImage, 54);
        addAndMakeVisible(bitReduction.get());
    }

    ~ReduxPanel() {}

    void resized() override
    {
        float center = getWidth() * 0.5;
        downsampleAmount->setBounds(center - downsampleAmount->getKnobSize() * 0.5 - 85, 0, downsampleAmount->getKnobSize(), downsampleAmount->getKnobHeight());
        bitReduction->setBounds(center - bitReduction->getKnobSize() * 0.5 + 85, 0, bitReduction->getKnobSize(), bitReduction->getKnobHeight());
    }

   

private:
    AudioPluginAudioProcessor &processorRef;

    std::unique_ptr<FilmStripKnob> downsampleAmount = nullptr;
    std::unique_ptr<FilmStripKnob> bitReduction = nullptr;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReduxPanel);
};