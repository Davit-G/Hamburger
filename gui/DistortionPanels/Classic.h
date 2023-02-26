#pragma once

#include <Juceheader.h>
#include "../../PluginProcessor.h"



class Classic : public juce::Component
{
public:
    Classic(AudioPluginAudioProcessor &p) : processorRef(p)
    {

        juce::Image smallKnobImage = juce::ImageCache::getFromMemory(BinaryData::ingain2_min_png, BinaryData::ingain2_min_pngSize);
        juce::Image knobImage = juce::ImageCache::getFromMemory(BinaryData::knob_min_png, BinaryData::knob_min_pngSize);
        juce::Image sizzleImage = juce::ImageCache::getFromMemory(BinaryData::sizzleknob_min_png, BinaryData::sizzleknob_min_pngSize);

        saturationAmount = std::make_unique<FilmStripKnob>(p, "saturationAmount", "Saturation", 0.0f, 100.0f, sizzleImage, 72);
        addAndMakeVisible(saturationAmount.get());

        fuzz = std::make_unique<FilmStripKnob>(p, "fuzz", "Fuzz", 0.0f, 100.0f, knobImage, 54);
        addAndMakeVisible(fuzz.get());

        bias = std::make_unique<FilmStripKnob>(p, "bias", "Bias", -1.0f, 1.0f, knobImage, 54);
        addAndMakeVisible(bias.get());

        fold = std::make_unique<FilmStripKnob>(p, "fold", "Fold", 0.0f, 100.0f, knobImage, 54);
        addAndMakeVisible(fold.get());
    }

    ~Classic() {}

    void resized() override
    {
        float center = getWidth() * 0.5 - 20;
        saturationAmount->setBounds(center - saturationAmount->getKnobSize() * 0.5 - 85, 5, saturationAmount->getKnobSize(), saturationAmount->getKnobHeight());
        fuzz->setBounds(center - fuzz->getKnobSize() * 0.5, 15, fuzz->getKnobSize(), fuzz->getKnobHeight());
        bias->setBounds(center - bias->getKnobSize() * 0.5 + 85, 15, bias->getKnobSize(), bias->getKnobHeight());
        fold->setBounds(center - fold->getKnobSize() * 0.5 + 155, 15, fold->getKnobSize(), fold->getKnobHeight());
    
    }

private:
    AudioPluginAudioProcessor &processorRef;

    std::unique_ptr<FilmStripKnob> saturationAmount = nullptr;
    std::unique_ptr<FilmStripKnob> fuzz = nullptr;
    std::unique_ptr<FilmStripKnob> bias = nullptr;
    std::unique_ptr<FilmStripKnob> fold = nullptr;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Classic);
};