#pragma once

#include <Juceheader.h>
#include "../../PluginProcessor.h"



class ReverbPanel : public juce::Component
{
public:
    ReverbPanel(AudioPluginAudioProcessor &p) : processorRef(p)
    {

        juce::Image smallKnobImage = juce::ImageCache::getFromMemory(BinaryData::ingain2_min_png, BinaryData::ingain2_min_pngSize);
        juce::Image knobImage = juce::ImageCache::getFromMemory(BinaryData::knob_min_png, BinaryData::knob_min_pngSize);
        juce::Image sizzleImage = juce::ImageCache::getFromMemory(BinaryData::sizzleknob_min_png, BinaryData::sizzleknob_min_pngSize);

        reverbSize = std::make_unique<FilmStripKnob>(p, "reverbSize", "Size", 0.0f, 1.0f, sizzleImage, 72);
        addAndMakeVisible(reverbSize.get());

        reverbWidth = std::make_unique<FilmStripKnob>(p, "reverbWidth", "Width", 0.00f, 1.0f, knobImage, 54);
        addAndMakeVisible(reverbWidth.get());

        reverbDamping = std::make_unique<FilmStripKnob>(p, "reverbDamping", "Damping", 0.0f, 1.0f, knobImage, 54);
        addAndMakeVisible(reverbDamping.get());

        reverbMix = std::make_unique<FilmStripKnob>(p, "reverbMix", "Mix", 0.0f, 100.0f, knobImage, 54);
        addAndMakeVisible(reverbMix.get());
    }

    ~ReverbPanel() {}

    void resized() override
    {
        float center = getWidth() * 0.5 - 20;
        reverbSize->setBounds(center - reverbSize->getKnobSize() * 0.5 - 85, 5, reverbSize->getKnobSize(), reverbSize->getKnobHeight());
        reverbWidth->setBounds(center - reverbWidth->getKnobSize() * 0.5, 15, reverbWidth->getKnobSize(), reverbWidth->getKnobHeight());
        reverbDamping->setBounds(center - reverbDamping->getKnobSize() * 0.5 + 85, 15, reverbDamping->getKnobSize(), reverbDamping->getKnobHeight());
        reverbMix->setBounds(center - reverbMix->getKnobSize() * 0.5 + 155, 15, reverbMix->getKnobSize(), reverbMix->getKnobHeight());

    
    }

private:
    AudioPluginAudioProcessor &processorRef;

    std::unique_ptr<FilmStripKnob> reverbSize = nullptr;
    std::unique_ptr<FilmStripKnob> reverbWidth = nullptr;
    std::unique_ptr<FilmStripKnob> reverbDamping = nullptr;
    std::unique_ptr<FilmStripKnob> reverbMix = nullptr;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbPanel);
};