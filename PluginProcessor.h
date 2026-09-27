#pragma once

#include "juce_core/juce_core.h"
#include "juce_dsp/juce_dsp.h"
#include "juce_audio_processors/juce_audio_processors.h"

#include "dsp/OversamplingStack.h"

#include "dsp/PrimaryDistortion.h"
#include "dsp/LevelStage.h"
#include "dsp/MainRouting.h"
#include "dsp/NoiseDistortions.h"
#include "dsp/PreDistortions/PreDistortion.h"
#include "dsp/Dynamics/Dynamics.h"
#include "dsp/Distortions/PostClip.h"

#include "gui/Modules/Scope.h"

#include "dsp/Filtering/EmphasisFilter.h"

#include "dsp/Fifo.h"
#include "service/PresetManager.h"
#include "service/AppProperties.h"

#include "gui/Modules/ScopeDataCollector.h"

// profiling
// #include <melatonin_perfetto/melatonin_perfetto.h>

#include "clap-juce-extensions/clap-juce-extensions.h"

#include "dsp/EffectBase.h"
#include "utils/Params.h"

//==============================================================================
class AudioPluginAudioProcessor : public juce::AudioProcessor, public clap_juce_extensions::clap_properties,
                                  private juce::ValueTree::Listener

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

    ScopeDataCollector<float>& getScopeDataCollector() {return scopeDataCollector; };

    RoutingOrder getRoutingOrder() const { return unpackRouting (order.load (std::memory_order_acquire)); }

    // use in message thread
    void setRoutingOrder (const RoutingOrder& newOrder);

    ScopeContext& getScopeContext() { return scopeContext; };
    Preset::PresetManager& getPresetManager() { return *presetManager; }
    AppProperties& getAppProperties() { return appProperties; }

private:
    juce::AudioParameterFloat *inputGainKnob = nullptr;
    juce::AudioParameterFloat *mixKnob = nullptr;
    juce::AudioParameterFloat *outputGainKnob = nullptr;
    juce::AudioParameterBool *gainLink = nullptr;

    juce::AudioParameterInt *hq = nullptr;
    juce::AudioParameterBool *hamburgerEnabledButton = nullptr;

    juce::AudioParameterChoice *oversamplingFactor = nullptr;

    EmphasisFilter emphasisFilter; // base effect

    EmphasisEffectFilter emphasisPreFilter { emphasisFilter, false };
    Dynamics dynamics;
    PreDistortion preDistortionSelection;
    NoiseDistortions noiseDistortionSelection;
    MainRouting distortionTypeSelection;
    EmphasisEffectFilter emphasisPostFilter { emphasisFilter, true };
    PostClip postClip;

    PrimaryDistortion preDistortion;
    PrimaryDistortion postDistortion;


    juce::dsp::Gain<float> inputGain;
    juce::dsp::Gain<float> outputGain;

    juce::dsp::DryWetMixer<float> dryWetMixer { 16384 };
    DryPhase dryPhase;
    bool linearCrossovers = false;
    float latencyTotal = 0.0f;

    void updateLatency();
    float totalLatency();

    OversamplingStack oversamplingStack;

    int oldOversamplingFactor = 0;

    ScopeDataCollector<float> scopeDataCollector;

    ScopeContext scopeContext; // keep on audio thread cause uhh idk its easier to access via reference from pluginprocessor

    AppProperties appProperties;
    std::unique_ptr<Preset::PresetManager> presetManager;

    #if PERFETTO
        std::unique_ptr<perfetto::TracingSession> tracingSession;
    #endif

    // packRouting/unpackRouting in Params.h say why this is a uint64 rather than the array
    std::atomic<juce::uint64> order { packRouting (defaultRouting) };
    
    static inline const juce::Identifier routingOrderProperty { "routingOrder" };
    void syncRoutingOrder();
    void valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeRedirected (juce::ValueTree& tree) override;

    // makes sure that routingOrder doesnt get converted into a mutex under the hood
    static_assert (std::atomic<juce::uint64>::is_always_lock_free);

    std::array<EffectBase*, (size_t) ModuleId::count> slots;

    // in, dry/wet and out around the modules that don't have their own inside them like the distortions do
    LevelStage dynamicsLevels { treeState, { ModuleId::dynamics, 0 } };
    LevelStage noiseLevels { treeState, { ModuleId::module1, 0 } };
    LevelStage preFxLevels { treeState, { ModuleId::module2, 0 } };
    std::array<LevelStage*, (size_t) ModuleId::count> slotLevels {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};