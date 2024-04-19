#pragma once

#include "juce_core/juce_core.h"
#include "juce_dsp/juce_dsp.h"
#include "juce_audio_processors/juce_audio_processors.h"

#include "dsp/OversamplingStack.h"

#include "dsp/PrimaryDistortion.h"
#include "dsp/NoiseDistortions.h"
#include "dsp/PreDistortions/PreDistortion.h"
#include "dsp/Dynamics/Dynamics.h"
#include "dsp/Distortions/PostClip.h"

#include "gui/Modules/Scope.h"

#include "dsp/Fifo.h"
#include "service/PresetManager.h"

// #include "dsp/ProcessDuplicator.h"

// profiling
#include <melatonin_perfetto/melatonin_perfetto.h>

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

    Preset::PresetManager& getPresetManager() { return *presetManager; }

private:
    juce::AudioParameterFloat *inputGainKnob = nullptr;
    juce::AudioParameterFloat *mixKnob = nullptr;
    juce::AudioParameterFloat *outputGainKnob = nullptr;

    juce::AudioParameterBool *clipEnabled = nullptr;
    juce::AudioParameterBool *enableEmphasis = nullptr;
    juce::AudioParameterInt *hq = nullptr;
    juce::AudioParameterBool *hamburgerEnabledButton = nullptr;

    juce::AudioParameterFloat *emphasisLow = nullptr;
    juce::AudioParameterFloat *emphasisHigh = nullptr;
    juce::AudioParameterFloat *emphasisLowFreq = nullptr;
    juce::AudioParameterFloat *emphasisHighFreq = nullptr;

    juce::AudioParameterChoice *oversamplingFactor = nullptr;

    juce::AudioParameterFloat *emphasis[2];
    juce::AudioParameterFloat *emphasisFreq[2];
    // float prevEmphasis[2] = {0.f, 0.f};
    // float prevEmphasisFreq[2] = {0.f, 0.f};

    PreDistortion preDistortionSelection;
    PrimaryDistortion distortionTypeSelection;
    NoiseDistortions noiseDistortionSelection;
    Dynamics dynamics;
    PostClip postClip;

    // SIMDGain simdGain;

    SmoothParam emphasisLowSmooth;
    SmoothParam emphasisHighSmooth;
    SmoothParam emphasisLowFreqSmooth;
    SmoothParam emphasisHighFreqSmooth;

    std::vector<float> emphasisLowBuffer;
    std::vector<float> emphasisHighBuffer;
    std::vector<float> emphasisLowFreqBuffer;
    std::vector<float> emphasisHighFreqBuffer;

    dsp::IIR::Filter<float> peakFilterBefore[2][2];
    dsp::IIR::Filter<float> peakFilterAfter[2][2];

    float filterFrequencies[2] = {62.0f, 9000.0f};

    dsp::Gain<float> inputGain;
    dsp::Gain<float> outputGain;
    dsp::Gain<float> emphasisCompensationGain;

    dsp::DryWetMixer<float> dryWetMixer;

    OversamplingStack oversamplingStack;
    OversamplingStack oversamplingStackPost;

    int oldOversamplingFactor = 0;

    AudioBufferQueue<float> audioBufferQueueL;
    AudioBufferQueue<float> audioBufferQueueR;
    ScopeDataCollector<float> scopeDataCollector { audioBufferQueueL, audioBufferQueueR };

    std::unique_ptr<Preset::PresetManager> presetManager;

    #if PERFETTO // if we have the profiling
        std::unique_ptr<perfetto::TracingSession> tracingSession;
    #endif

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};
