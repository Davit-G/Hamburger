#pragma once

#include <Juceheader.h>
#include "../../PluginProcessor.h"

#include "../FilmStripKnob.h"

class ShiftPanel : public juce::Component
{
public:
    ShiftPanel(AudioPluginAudioProcessor &p) : processorRef(p)
    {

        juce::Image smallKnobImage = juce::ImageCache::getFromMemory(BinaryData::ingain2_min_png, BinaryData::ingain2_min_pngSize);
        juce::Image knobImage = juce::ImageCache::getFromMemory(BinaryData::knob_min_png, BinaryData::knob_min_pngSize);
        juce::Image sizzleImage = juce::ImageCache::getFromMemory(BinaryData::sizzleknob_min_png, BinaryData::sizzleknob_min_pngSize);

        freqShiftAmount = std::make_unique<FilmStripKnob>(p, "frequencyShiftFreq", "FreqShift", -500, 500.0f, sizzleImage, 72);
        addAndMakeVisible(freqShiftAmount.get());
        // emphasisMidKnob = std::make_unique<FilmStripKnob>(p, "emphasisMidGain", "Emphasis Mid Gain", -18.0f, 18.0f, smallKnobImage, 38);
        // addAndMakeVisible(emphasisMidKnob.get());
    }

    ~ShiftPanel() {}

    void resized() override
    {
        freqShiftAmount->setBounds(20, 20, 100, 100);
        // emphasisMidKnob->setBounds(660 - 604, 202 - 138, emphasisMidKnob->getKnobSize(), emphasisMidKnob->getKnobSize());
    }

private:
    AudioPluginAudioProcessor &processorRef;

    std::unique_ptr<FilmStripKnob> freqShiftAmount = nullptr;
    // std::unique_ptr<FilmStripKnob> emphasisMidKnob = nullptr;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShiftPanel);
};