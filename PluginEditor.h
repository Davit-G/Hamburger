#pragma once

#include "PluginProcessor.h"
// #include "gui/ParameterKnob.h"
#include "gui/FilmStripKnob.h"

#include "gui/LightButton.h"

#include <JuceHeader.h>

#include "gui/Emphasis/EmphasisPanel.h"
#include "gui/PreDistortionPanels/PreDistortions.h"
#include "gui/DistortionPanels/Distortions.h"
#include "gui/NoisePanels/NoisePanel.h"

#include "./gui/LookAndFeel/ComboBoxLookAndFeel.h"


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

    // using new layout system


    // primary distortions
    std::unique_ptr<FilmStripKnob> saturationKnob = nullptr;

    // noise distortions
    std::unique_ptr<FilmStripKnob> noiseAmountKnob = nullptr;
    std::unique_ptr<FilmStripKnob> noiseFrequencyKnob = nullptr;
    std::unique_ptr<FilmStripKnob> noiseQKnob = nullptr;

    

    std::unique_ptr<LightButton> companderButton = nullptr;
    std::unique_ptr<LightButton> emphasisButton = nullptr;
    std::unique_ptr<LightButton> compressorButton = nullptr;
    std::unique_ptr<LightButton> expanderButton = nullptr;

    std::unique_ptr<LightButton> bypassButton = nullptr;
    std::unique_ptr<LightButton> autoGainButton = nullptr;


    EmphasisPanel emphasisPanel;
    PreDistortions preDistortionsPanel;
    DistortionsPanel distortionsPanel;
    NoisePanel noiseTypesPanel;

    ComboBoxLookAndFeel comboBoxLook;


    juce::ComboBox oversamplingComboBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> oversamplingAttachment;

    juce::ComboBox distortionTypeComboBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> distortionTypesAttachment;

    juce::Image background;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};
