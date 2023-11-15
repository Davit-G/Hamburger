#pragma once

#include <Juceheader.h>
#include "../../PluginProcessor.h"

class SizzlePanel : public juce::Component
{
public:
    SizzlePanel(AudioPluginAudioProcessor &p) : processorRef(p)
    {
        juce::Image sizzleImage = juce::ImageCache::getFromMemory(BinaryData::sizzleknob_min_png, BinaryData::sizzleknob_min_pngSize);

        sizzleAmount = std::make_unique<FilmStripKnob>(p, "noiseAmount", "Amount", 0.0f, 100.0f, sizzleImage, 72);
        addAndMakeVisible(sizzleAmount.get());
    }

    ~SizzlePanel() {}

    void resized() override
    {
        float center = getWidth() * 0.5;
        sizzleAmount->setBounds(center - sizzleAmount->getKnobSize() * 0.5, 0, sizzleAmount->getKnobSize(), sizzleAmount->getKnobHeight());
    }

   

private:
    AudioPluginAudioProcessor &processorRef;

    std::unique_ptr<FilmStripKnob> sizzleAmount = nullptr;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SizzlePanel);
};