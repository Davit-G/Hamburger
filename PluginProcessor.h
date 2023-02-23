#pragma once

#include <JuceHeader.h>

#include "dsp/Compander.h"
#include "dsp/Distortions/Fuzz.h"
#include "dsp/Distortions/Sizzle.h"
#include "dsp/Distortions/Cooked.h"
#include "dsp/Gain.h"
#include "dsp/OversamplingStack.h"
#include "dsp/Distortions/SoftClipper.h"

#include "dsp/PrimaryDistortion.h"
#include "dsp/NoiseDistortions.h"
#include "dsp/PreDistortion.h"

#include "dsp/FrequencyShifting/FrequencyShifterAlt.h"
// #include "dsp/FrequencyShifting/FreqShiftBiquad.h"
// #include "dsp/FrequencyShifting/WeaverShifter.h"

// #include "Distortions/naivetube/NaiveTubeDistortion.h"

//==============================================================================
class AudioPluginAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String &newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState treeState;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::StringArray oversamplingFactorChoices = {"1x", "2x", "4x", "8x", "16x"};

    

private:
    juce::AudioParameterFloat *knobValue = nullptr;
    juce::AudioParameterFloat *inputGainKnob = nullptr;
    juce::AudioParameterFloat *mixKnob = nullptr;
    juce::AudioParameterFloat *outputGainKnob = nullptr;

    juce::AudioParameterBool *enableCompander = nullptr;
    juce::AudioParameterBool *enableEmphasis = nullptr;
    juce::AudioParameterBool *enableCompressor = nullptr;
    juce::AudioParameterBool *enableExpander = nullptr;

    juce::AudioParameterBool *hamburgerEnabledButton = nullptr; // acts as bypass

    juce::AudioParameterFloat *emphasisLow = nullptr;
    juce::AudioParameterFloat *emphasisMid = nullptr;
    juce::AudioParameterFloat *emphasisHigh = nullptr;
    juce::AudioParameterFloat *emphasisLowFreq = nullptr;
    juce::AudioParameterFloat *emphasisMidFreq = nullptr;
    juce::AudioParameterFloat *emphasisHighFreq = nullptr;

    juce::AudioParameterFloat *freqShiftFreq = nullptr;


    juce::AudioParameterChoice *oversamplingFactor = nullptr;

    int oldOversamplingFactor = -1;

    // array of emphasisLow, emphasisMid, emphasisHigh
    juce::AudioParameterFloat *emphasis[3];
    juce::AudioParameterFloat *emphasisFreq[3];
    float prevEmphasis[3] = {0.f, 0.f, 0.f};
    float prevEmphasisFreq[3] = {0.f, 0.f, 0.f};

    juce::AudioParameterFloat *saturation = nullptr;

    Compander compander;

    FrequencyShifterAlt shifter;

    // NaiveTubeDistortion tubeDistortion;

    // Fuzz pattyDistortion;
    // Sizzle sizzleNoise;
    // Cooked cookedDistortion;

    PreDistortion preDistortionSelection;
    PrimaryDistortion distortionTypeSelection;
    NoiseDistortions noiseDistortionSelection;

    dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> peakFilterBefore[3];
    dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> peakFilterAfter[3];

    float filterFrequencies[3] = {62.0f, 1220.0f, 9000.0f};

    double oversampledSampleRate;

    dsp::Gain<float> inputGain;
    dsp::Gain<float> outputGain;
    dsp::Gain<float> emphasisCompensationGain;

    dsp::DryWetMixer<float> dryWetMixer;

    dsp::Oversampling<float> oversampling;

    OversamplingStack oversamplingStack;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};
