#pragma once

#include <JuceHeader.h>

#include "dsp/Compressor.h"
#include "dsp/Distortions/Patty.h"
#include "dsp/Distortions/Sizzle.h"
#include "dsp/Distortions/Cooked.h"
// #include "Distortions/naivetube/NaiveTubeDistortion.h"

//==============================================================================
class AudioPluginAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
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
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState treeState;

	juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    juce::AudioParameterFloat* knobValue = nullptr;
    juce::AudioParameterFloat* inputGainKnob = nullptr;
    juce::AudioParameterBool* enableCompander = nullptr;
    juce::AudioParameterBool* enableEmphasis = nullptr;

    juce::AudioParameterFloat* emphasisLow = nullptr;
    juce::AudioParameterFloat* emphasisMid = nullptr;
    juce::AudioParameterFloat* emphasisHigh = nullptr;

    // array of emphasisLow, emphasisMid, emphasisHigh
    juce::AudioParameterFloat* emphasis[3];

    juce::AudioParameterFloat* saturation = nullptr;

    Dynamics compressor;
    Dynamics compressorEnd;
    Dynamics expander;

    // NaiveTubeDistortion tubeDistortion;

    Patty pattyDistortion;
    Sizzle sizzleNoise;
    Cooked cookedDistortion;


    dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> peakFilterBefore[3];
    dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> peakFilterAfter[3];

    float filterFrequencies[3] = { 62.0f, 1220.0f, 6000.0f };




    double cachedSampleRate;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
