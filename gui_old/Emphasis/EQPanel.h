#pragma once

#include <Juceheader.h>
#include "../../PluginProcessor.h"

class EQPanel : public juce::Component
{
public:
    EQPanel(AudioPluginAudioProcessor &p) : processorRef(p)
    {

        juce::Image smallKnobImage = juce::ImageCache::getFromMemory(BinaryData::ingain2_min_png, BinaryData::ingain2_min_pngSize);
        juce::Image knobImage = juce::ImageCache::getFromMemory(BinaryData::knob_min_png, BinaryData::knob_min_pngSize);
        juce::Image sizzleImage = juce::ImageCache::getFromMemory(BinaryData::sizzleknob_min_png, BinaryData::sizzleknob_min_pngSize);

        emphasisLowKnob = std::make_unique<FilmStripKnob>(p, "emphasisLowGain", "Lvl", -18.0f, 18.0f, smallKnobImage, 38);
        addAndMakeVisible(emphasisLowKnob.get());
        emphasisMidKnob = std::make_unique<FilmStripKnob>(p, "emphasisMidGain", "Lvl", -18.0f, 18.0f, smallKnobImage, 38);
        addAndMakeVisible(emphasisMidKnob.get());
        emphasisHighKnob = std::make_unique<FilmStripKnob>(p, "emphasisHighGain", "Lvl", -18.0f, 18.0f, smallKnobImage, 38);
        addAndMakeVisible(emphasisHighKnob.get());
        emphasisLowFreqKnob = std::make_unique<FilmStripKnob>(p, "emphasisLowFreq", "Lo", 30.0f, 200.0f, smallKnobImage, 38);
        addAndMakeVisible(emphasisLowFreqKnob.get());
        emphasisMidFreqKnob = std::make_unique<FilmStripKnob>(p, "emphasisMidFreq", "Mid", 500.0f, 3000.0f, smallKnobImage, 38);
        addAndMakeVisible(emphasisMidFreqKnob.get());
        emphasisHighFreqKnob = std::make_unique<FilmStripKnob>(p, "emphasisHighFreq", "Hi", 6000.0f, 18000.0f, smallKnobImage, 38);
        addAndMakeVisible(emphasisHighFreqKnob.get());
    }

    ~EQPanel() {}

    void resized() override
    {
        emphasisLowKnob->setBounds(604 - 604, 202 - 138, emphasisLowKnob->getKnobSize(), emphasisLowKnob->getKnobHeight());
        emphasisMidKnob->setBounds(660 - 604, 202 - 138, emphasisMidKnob->getKnobSize(), emphasisMidKnob->getKnobHeight());
        emphasisHighKnob->setBounds(717 - 604, 202 - 138, emphasisHighKnob->getKnobSize(), emphasisHighKnob->getKnobHeight());
        emphasisLowFreqKnob->setBounds(603 - 604, 138 - 138, emphasisLowFreqKnob->getKnobSize(), emphasisLowFreqKnob->getKnobHeight());
        emphasisMidFreqKnob->setBounds(659 - 604, 138 - 138, emphasisMidFreqKnob->getKnobSize(), emphasisMidFreqKnob->getKnobHeight());
        emphasisHighFreqKnob->setBounds(716 - 604, 138 - 138, emphasisHighFreqKnob->getKnobSize(), emphasisHighFreqKnob->getKnobHeight());
    }

private:
    AudioPluginAudioProcessor &processorRef;

    std::unique_ptr<FilmStripKnob> emphasisLowKnob = nullptr;
    std::unique_ptr<FilmStripKnob> emphasisMidKnob = nullptr;
    std::unique_ptr<FilmStripKnob> emphasisHighKnob = nullptr;
    std::unique_ptr<FilmStripKnob> emphasisLowFreqKnob = nullptr;
    std::unique_ptr<FilmStripKnob> emphasisMidFreqKnob = nullptr;
    std::unique_ptr<FilmStripKnob> emphasisHighFreqKnob = nullptr;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EQPanel);
};