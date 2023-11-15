#pragma once

#include <Juceheader.h>
#include "../../PluginProcessor.h"

class CombPanel : public juce::Component
{
public:
    CombPanel(AudioPluginAudioProcessor &p) : processorRef(p)
    {
        juce::Image smallKnobImage = juce::ImageCache::getFromMemory(BinaryData::ingain2_min_png, BinaryData::ingain2_min_pngSize);
        juce::Image knobImage = juce::ImageCache::getFromMemory(BinaryData::knob_min_png, BinaryData::knob_min_pngSize);
        juce::Image sizzleImage = juce::ImageCache::getFromMemory(BinaryData::sizzleknob_min_png, BinaryData::sizzleknob_min_pngSize);

        combDelay = std::make_unique<FilmStripKnob>(p, "combDelay", "Delay", 0.01f, 2000.0f, sizzleImage, 72);
        addAndMakeVisible(combDelay.get());

        combFeedback = std::make_unique<FilmStripKnob>(p, "combFeedback", "Feedback", -1.0f, 1.0f, knobImage, 54);
        addAndMakeVisible(combFeedback.get());

        combMix = std::make_unique<FilmStripKnob>(p, "combMix", "Mix", 0.0f, 100.0f, knobImage, 54);
        addAndMakeVisible(combMix.get());
    }

    ~CombPanel() {}

    void resized() override
    {
        float center = getWidth() * 0.5;
        combDelay->setBounds(center - combDelay->getKnobSize() * 0.5, 5, combDelay->getKnobSize(), combDelay->getKnobHeight());
        combFeedback->setBounds(center - combFeedback->getKnobSize() * 0.5 - 85, 15, combFeedback->getKnobSize(), combFeedback->getKnobHeight());
        combMix->setBounds(center - combMix->getKnobSize() * 0.5 + 85, 15, combMix->getKnobSize(), combMix->getKnobHeight());
    }

private:
    AudioPluginAudioProcessor &processorRef;

    std::unique_ptr<FilmStripKnob> combDelay = nullptr;
    std::unique_ptr<FilmStripKnob> combFeedback = nullptr;
    std::unique_ptr<FilmStripKnob> combMix = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CombPanel);
};