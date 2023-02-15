#pragma once

#include "PluginProcessor.h"
#include "gui/ParameterKnob.h"
#include "gui/FilmStripKnob.h"

#include "gui/LightButton.h"

#include <JuceHeader.h>

//==============================================================================
class AudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor &);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics &) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioPluginAudioProcessor &processorRef;

    std::unique_ptr<FilmStripKnob> inputGainKnob = nullptr;
    std::unique_ptr<FilmStripKnob> outputGainKnob = nullptr;
    std::unique_ptr<FilmStripKnob> mixKnob = nullptr;

    std::unique_ptr<FilmStripKnob> compAttackKnob = nullptr;
    std::unique_ptr<FilmStripKnob> compReleaseKnob = nullptr;
    std::unique_ptr<FilmStripKnob> compThresholdKnob = nullptr;
    std::unique_ptr<FilmStripKnob> compRatioKnob = nullptr;
    std::unique_ptr<FilmStripKnob> compOutKnob = nullptr;

    std::unique_ptr<FilmStripKnob> fuzzKnob = nullptr;
    std::unique_ptr<FilmStripKnob> sizzleKnob = nullptr;
    std::unique_ptr<FilmStripKnob> foldKnob = nullptr;
    std::unique_ptr<FilmStripKnob> saturationKnob = nullptr;

    std::unique_ptr<FilmStripKnob> emphasisLowKnob = nullptr;
    std::unique_ptr<FilmStripKnob> emphasisMidKnob = nullptr;
    std::unique_ptr<FilmStripKnob> emphasisHighKnob = nullptr;
    std::unique_ptr<FilmStripKnob> emphasisLowFreqKnob = nullptr;
    std::unique_ptr<FilmStripKnob> emphasisMidFreqKnob = nullptr;
    std::unique_ptr<FilmStripKnob> emphasisHighFreqKnob = nullptr;

    std::unique_ptr<LightButton> companderButton = nullptr;
    std::unique_ptr<LightButton> emphasisButton = nullptr;
    std::unique_ptr<LightButton> compressorButton = nullptr;
    std::unique_ptr<LightButton> expanderButton = nullptr;

    std::unique_ptr<LightButton> bypassButton = nullptr;
    std::unique_ptr<LightButton> autoGainButton = nullptr;


    juce::ComboBox oversamplingComboBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> oversamplingAttachment;

    juce::Image background;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};
