#pragma once

#include "../PluginProcessor.h"
#include "FilmStripKnob.h"
#include "LightButton.h"

#include <JuceHeader.h>

#include "Emphasis/EmphasisPanel.h"
#include "PreDistortionPanels/PreDistortions.h"
#include "DistortionPanels/Distortions.h"
#include "NoisePanels/NoisePanel.h"

#include "LookAndFeel/ComboBoxLookAndFeel.h"


//==============================================================================
class EditorV1 : public juce::AudioProcessorEditor
{
public:
    explicit EditorV1(AudioPluginAudioProcessor &);
    ~EditorV1() override;

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorV1)
};
