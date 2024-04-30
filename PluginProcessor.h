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

#include "dsp/Filtering/EmphasisFilter.h"

#include "dsp/Fifo.h"
#include "service/PresetManager.h"

// profiling
#include <melatonin_perfetto/melatonin_perfetto.h>

#include "clap-juce-extensions/clap-juce-extensions.h"

#if SENTRY
#include <sentry.h>
#endif


//==============================================================================
class AudioPluginAudioProcessor : public juce::AudioProcessor, public clap_juce_extensions::clap_properties

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
    juce::AudioParameterInt *hq = nullptr;
    juce::AudioParameterBool *hamburgerEnabledButton = nullptr;

    juce::AudioParameterChoice *oversamplingFactor = nullptr;

    PreDistortion preDistortionSelection;
    PrimaryDistortion distortionTypeSelection;
    NoiseDistortions noiseDistortionSelection;
    Dynamics dynamics;
    PostClip postClip;

    EmphasisFilter emphasisFilter;

    dsp::Gain<float> inputGain;
    dsp::Gain<float> outputGain;

    dsp::DryWetMixer<float> dryWetMixer;

    OversamplingStack oversamplingStack;

    int oldOversamplingFactor = 0;

    AudioBufferQueue<float> audioBufferQueueL;
    AudioBufferQueue<float> audioBufferQueueR;
    ScopeDataCollector<float> scopeDataCollector { audioBufferQueueL, audioBufferQueueR };

    std::unique_ptr<Preset::PresetManager> presetManager;

    #if PERFETTO
        std::unique_ptr<perfetto::TracingSession> tracingSession;
    #endif

    #if SENTRY

    static void createSentryLogger(void *platformSpecificCrashData) {
        auto report = juce::SystemStats::getStackBacktrace();

        sentry_value_t event = sentry_value_new_event();
        sentry_value_set_by_key(event, "message", sentry_value_new_string(report.toRawUTF8()));
        sentry_capture_event(event);

        sentry_options_free(AudioPluginAudioProcessor::options);
        sentry_shutdown();
    }
    
    static sentry_options_t *options;

    #endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};

#if SENTRY
sentry_options_t *AudioPluginAudioProcessor::options = nullptr;
#endif