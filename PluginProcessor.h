#pragma once

#include <JuceHeader.h>

#include "dsp/OversamplingStack.h"

#include "dsp/PrimaryDistortion.h"
#include "dsp/NoiseDistortions.h"
#include "dsp/PreDistortions/PreDistortion.h"
#include "dsp/Dynamics/Dynamics.h"
#include "dsp/Distortions/PostClip.h"

#include "gui/Modules/Scope.h"

#include "dsp/Fifo.h"

// #include "xsimd/xsimd.hpp"
// #include "dsp/SIMDGain.h"

// profiling
// #include <melatonin_perfetto/melatonin_perfetto.h>

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

    AudioBufferQueue<float>& getAudioBufferQueueL() noexcept        { return audioBufferQueueL; }
    AudioBufferQueue<float>& getAudioBufferQueueR() noexcept        { return audioBufferQueueR; }

private:
    // juce::AudioParameterFloat *knobValue = nullptr;
    juce::AudioParameterFloat *inputGainKnob = nullptr;
    juce::AudioParameterFloat *mixKnob = nullptr;
    juce::AudioParameterFloat *outputGainKnob = nullptr;

    juce::AudioParameterBool *enableEmphasis = nullptr;
    juce::AudioParameterBool *hamburgerEnabledButton = nullptr; // acts as bypass

    juce::AudioParameterFloat *emphasisLow = nullptr;
    juce::AudioParameterFloat *emphasisMid = nullptr;
    juce::AudioParameterFloat *emphasisHigh = nullptr;
    juce::AudioParameterFloat *emphasisLowFreq = nullptr;
    juce::AudioParameterFloat *emphasisMidFreq = nullptr;
    juce::AudioParameterFloat *emphasisHighFreq = nullptr;

    juce::AudioParameterChoice *oversamplingFactor = nullptr;

    juce::AudioParameterFloat *emphasis[3];
    juce::AudioParameterFloat *emphasisFreq[3];
    float prevEmphasis[3] = {0.f, 0.f, 0.f};
    float prevEmphasisFreq[3] = {0.f, 0.f, 0.f};

    PreDistortion preDistortionSelection;
    PrimaryDistortion distortionTypeSelection;
    NoiseDistortions noiseDistortionSelection;
    Dynamics dynamics;
    PostClip postClip;

    // SIMDGain simdGain;

    dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> peakFilterBefore[3];
    dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> peakFilterAfter[3];

    float filterFrequencies[3] = {62.0f, 1220.0f, 9000.0f};

    dsp::Gain<float> inputGain;
    dsp::Gain<float> outputGain;
    dsp::Gain<float> emphasisCompensationGain;

    dsp::DryWetMixer<float> dryWetMixer;

    AudioBufferQueue<float> audioBufferQueueL;
    AudioBufferQueue<float> audioBufferQueueR;
    ScopeDataCollector<float> scopeDataCollector { audioBufferQueueL, audioBufferQueueR };

    #if PERFETTO // if we have the profiling
        std::unique_ptr<perfetto::TracingSession> tracingSession;
    #endif

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};
