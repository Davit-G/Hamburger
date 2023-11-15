#pragma once

#include <Juceheader.h>
#include "../../PluginProcessor.h"

class DisperserPanel : public juce::Component
{
public:
    DisperserPanel(AudioPluginAudioProcessor &p) : processorRef(p)
    {

        juce::Image smallKnobImage = juce::ImageCache::getFromMemory(BinaryData::ingain2_min_png, BinaryData::ingain2_min_pngSize);
        juce::Image knobImage = juce::ImageCache::getFromMemory(BinaryData::knob_min_png, BinaryData::knob_min_pngSize);
        juce::Image sizzleImage = juce::ImageCache::getFromMemory(BinaryData::sizzleknob_min_png, BinaryData::sizzleknob_min_pngSize);

        allPassFreq = std::make_unique<FilmStripKnob>(p, "allPassFreq", "Freq", 20.0f, 20000.0f, sizzleImage, 72);
        addAndMakeVisible(allPassFreq.get());

        allPassQ = std::make_unique<FilmStripKnob>(p, "allPassQ", "Q", 0.01f, 1.41f, knobImage, 54);
        addAndMakeVisible(allPassQ.get());

        allPassAmount = std::make_unique<FilmStripKnob>(p, "allPassAmount", "Stages", 0.0f, 50.0f, knobImage, 54);
        addAndMakeVisible(allPassAmount.get());
    }

    ~DisperserPanel() {}

    void resized() override
    {
        float center = getWidth() * 0.5;
        allPassFreq->setBounds(center - allPassFreq->getKnobSize() * 0.5, 5, allPassFreq->getKnobSize(), allPassFreq->getKnobHeight());
        allPassQ->setBounds(center - allPassQ->getKnobSize() * 0.5 - 85, 15, allPassQ->getKnobSize(), allPassQ->getKnobHeight());
        allPassAmount->setBounds(center - allPassAmount->getKnobSize() * 0.5 + 85, 15, allPassAmount->getKnobSize(), allPassAmount->getKnobHeight());
    }

   

private:
    AudioPluginAudioProcessor &processorRef;

    std::unique_ptr<FilmStripKnob> allPassFreq = nullptr;
    std::unique_ptr<FilmStripKnob> allPassQ = nullptr;
    std::unique_ptr<FilmStripKnob> allPassAmount = nullptr;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DisperserPanel);
};