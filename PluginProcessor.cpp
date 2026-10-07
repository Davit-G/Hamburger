#include "PluginProcessor.h"
#include "utils/EffectInfo.h"
#include "gui/ResizableEditor.h"

#include <chrono>
#include <ctime>


//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor() : AudioProcessor(BusesProperties()
                                                                            .withInput("Input", juce::AudioChannelSet::stereo(), true)
                                                                            .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
                                                         treeState(*this, nullptr, "PARAMETER", createParameterLayout()),
                                                         dynamics(treeState, scopeDataCollector),
                                                         postClip(treeState, scopeDataCollector),
                                                         dryWetMixer(30),
                                                         noiseSelection(treeState),
                                                         mainDistortion(treeState),
                                                         preDistortion(treeState, SlotId{ModuleId::preDistortion, 0}),
                                                         postDistortion(treeState, SlotId{ModuleId::postDistortion, 0}),
                                                         prefxSelection(treeState),
                                                         emphasisFilter(treeState)
{
    treeState.state = juce::ValueTree("savedParams");

    slotLevels[(size_t) ModuleId::dynamics] = &dynamicsInOutMix;
    slotLevels[(size_t) ModuleId::module1] = &noiseInOutMix;
    slotLevels[(size_t) ModuleId::module2] = &preFxInOutMix;

    slots[(size_t) ModuleId::preEmphasis] = &emphasisPreFilter;
    slots[(size_t) ModuleId::postEmphasis] = &emphasisPostFilter;
    slots[(size_t) ModuleId::dynamics] = &dynamics;
    slots[(size_t) ModuleId::module1] = &noiseSelection;
    slots[(size_t) ModuleId::module2] = &prefxSelection;
    slots[(size_t) ModuleId::main] = &mainDistortion;
    slots[(size_t) ModuleId::postClip] = &postClip;
    slots[(size_t) ModuleId::preDistortion] = &preDistortion;
    slots[(size_t) ModuleId::postDistortion] = &postDistortion;

    inputGainKnob = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter(ParamIDs::inputGain.getParamID()));
    if (inputGainKnob == nullptr)
        jassertfalse;

    outputGainKnob = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter(ParamIDs::outputGain.getParamID()));
    if (outputGainKnob == nullptr)
        jassertfalse;

    mixKnob = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter(ParamIDs::mix.getParamID()));
    if (mixKnob == nullptr)
        jassertfalse;

    hamburgerEnabledButton = dynamic_cast<juce::AudioParameterBool *>(treeState.getParameter(ParamIDs::hamburgerEnabled.getParamID()));
    if (hamburgerEnabledButton == nullptr)
        jassertfalse;

    hq = dynamic_cast<juce::AudioParameterInt *>(treeState.getParameter(ParamIDs::oversamplingFactor.getParamID()));
    if (hq == nullptr)
        jassertfalse;

    presetManager = std::make_unique<Preset::PresetManager>(treeState, appProperties);

#if PERFETTO
    // MelatoninPerfetto::get().beginSession(300000);
#endif
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
#if PERFETTO
    // MelatoninPerfetto::get().endSession();
#endif
}

juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> collected;

    auto addBool = [&collected] (const ParamIDs::ParameterInfo& info, bool defaultValue)
    {
        collected.push_back (std::make_unique<juce::AudioParameterBool> (info.getParameterID(), info.displayName, defaultValue));
    };

    auto addInt = [&collected] (const ParamIDs::ParameterInfo& info)
    {
        collected.push_back (std::make_unique<juce::AudioParameterInt> (info.getParameterID(), info.displayName, (int) info.range.start,
                                                        (int) info.range.end, (int) info.defaultValue));
    };

    auto addChoice = [&collected] (const ParamIDs::ParameterInfo& info, const juce::StringArray& names)
    {
        collected.push_back(std::make_unique<juce::AudioParameterChoice> (info.getParameterID(), info.displayName, ParamIDs::withReservedSlots (names), (int) info.defaultValue));
    };

    // the parameters that belong to no slot
    for (const auto* fixed : { &ParamIDs::inputGain, &ParamIDs::outputGain, &ParamIDs::mix,
                               &ParamIDs::globalDrive, &ParamIDs::compAmount, &ParamIDs::eqStrength, &ParamIDs::noiseAmount, &ParamIDs::preFxAmount,
                               &ParamIDs::crossoverLow, &ParamIDs::crossoverMid, &ParamIDs::crossoverHigh,
                               &ParamIDs::stackGain, &ParamIDs::stackMix, &ParamIDs::msBalance,
                               &ParamIDs::emphasisTilt, &ParamIDs::emphasisLowGain, &ParamIDs::emphasisHighGain,
                               &ParamIDs::emphasisLowFreq, &ParamIDs::emphasisHighFreq })
        collected.push_back(std::make_unique<MacroParam> (*fixed));

    for (int i = 0; i < ParamIDs::numGlobalMacros; ++i)
        collected.push_back(std::make_unique<MacroParam> (*ParamIDs::globalMacros[i]));

    addBool (ParamIDs::gainLink, false);
    addBool (ParamIDs::hamburgerEnabled, true);
    addBool (ParamIDs::emphasisOn, true);
    addBool (ParamIDs::stackFlip, true);

    addInt (ParamIDs::oversamplingFactor);
    addInt (ParamIDs::bandCount);
    addInt (ParamIDs::stackCount);
    addInt (ParamIDs::slewType);

    addChoice (ParamIDs::mainRouting, ParamIDs::routingTypes);
    addChoice (ParamIDs::emphasisType, ParamIDs::emphasisTypes);

    for (int band = 0; band < ParamIDs::numBands; ++band)
    {
        addBool (*ParamIDs::bandMutes[band], false);
        addBool (*ParamIDs::bandSolos[band], false);
    }

    // every slot of every module: its on/off and type, its in, dry/wet and out, and its own copy of every type's controls
    for (const auto& m : pluginModules)
    {
        for (int sub = 0; sub < m.slotCount; ++sub)
        {
            const SlotId slot { m.id, sub };

            collected.push_back(std::make_unique<juce::AudioParameterBool> (slot.enabled(), slot.displayNamePrefix() + " ENABLED", m.enabledByDefault));

            if (const auto* types = ParamIDs::categoriesFor (m.id))
                collected.push_back(std::make_unique<juce::AudioParameterChoice> (slot.type(), slot.displayNamePrefix() + " TYPE",
                                                                   ParamIDs::withReservedSlots (*types), 0));

            // the clipper has no levels of its own, whichever type it is running
            if (ParamIDs::categoriesFor (m.id) != nullptr && m.id != ModuleId::postClip)
            {
                for (const auto* level : { &ParamIDs::slotInGain, &ParamIDs::slotMix, &ParamIDs::slotOutGain })
                    collected.push_back(std::make_unique<MacroParam> (slot, *level));

                collected.push_back(std::make_unique<juce::AudioParameterBool> (paramIdFor (slot, ParamIDs::gainLink),
                                                                 slot.displayNamePrefix() + " " + ParamIDs::gainLink.displayName, false));
            }

            // types share controls, like the compressors' ratio, so each is only made once per slot
            juce::StringArray alreadyAdded;

            for (const auto* layout : EffectInfos::layoutsFor (m.id))
            {
                for (const auto& descriptor : layout->params)
                {
                    if (descriptor.id.isNull() || alreadyAdded.contains (descriptor.getParamID()))
                        continue;

                    alreadyAdded.add (descriptor.getParamID());
                    collected.push_back(std::make_unique<MacroParam> (slot, descriptor));
                }
            }
        }
    }

    // see VERSION HINTS in ParamIDs
    std::stable_sort (collected.begin(), collected.end(),
                      [] (const auto& a, const auto& b) { return a->getVersionHint() < b->getVersionHint(); });

    DBG ("parameters: " << (int) collected.size());

    if constexpr (ParamIDs::releasedParameterCount > 0)
    {
        const auto released = std::count_if (collected.begin(), collected.end(),
                                             [] (const auto& p) { return p->getVersionHint() <= ParamIDs::releasedVersionHint; });

        // a parameter added since the last release still has a released version hint, or a released parameter was
        // removed. either breaks Logic and GarageBand automation
        jassert (released == ParamIDs::releasedParameterCount);
        juce::ignoreUnused (released);
    }

    juce::AudioProcessorValueTreeState::ParameterLayout params;

    for (auto& parameter : collected)
        params.add(std::move (parameter));

    return params;
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const { return false; }
bool AudioPluginAudioProcessor::producesMidi() const { return false; }
bool AudioPluginAudioProcessor::isMidiEffect() const { return false; }
double AudioPluginAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int AudioPluginAudioProcessor::getNumPrograms() { return 1; } // some daws dont cope well etc etc, report 1 even if we dont have programs
int AudioPluginAudioProcessor::getCurrentProgram() { return 0; }
void AudioPluginAudioProcessor::setCurrentProgram(int index) { juce::ignoreUnused(index); }
const juce::String AudioPluginAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}
void AudioPluginAudioProcessor::changeProgramName(int index, const juce::String &newName) { juce::ignoreUnused(index, newName); }

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    if (sampleRate <= 0.0 || samplesPerBlock <= 0)
        return;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    inputGain.prepare(spec);
    outputGain.prepare(spec);


    const auto oversamplingFactor = (hq != nullptr) ? hq->get() : 0;
    oversamplingStack.setOversamplingFactor(oversamplingFactor);
    oversamplingStack.prepare(spec);

    juce::dsp::ProcessSpec oversampledSpec;
    oversampledSpec.sampleRate = sampleRate * pow(2, oversamplingStack.getOversamplingFactor());
    oversampledSpec.maximumBlockSize = samplesPerBlock * pow(2, oversamplingStack.getOversamplingFactor());
    oversampledSpec.numChannels = getTotalNumOutputChannels();

    // the filter both emphasis slots share, then everything the order can reach, so nothing in it runs unprepared
    emphasisFilter.prepare(oversampledSpec);

    for (auto* slot : slots)
        slot->prepare(oversampledSpec);

    for (auto* levels : slotLevels)
        if (levels != nullptr)
            levels->prepare(oversampledSpec);

    float totalLatency = oversamplingStack.getLatencySamples();

    DBG("Total Latency: " << totalLatency);

    setLatencySamples((int)std::ceil(totalLatency));

    dryWetMixer.reset();
    dryWetMixer.prepare(spec);
    dryWetMixer.setWetLatency(totalLatency);

    scopeDataCollector.prepare(spec);
}

void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                             juce::MidiBuffer &midiMessages)
{
    if (hamburgerEnabledButton != nullptr && hamburgerEnabledButton->get() == false)
        return;

    juce::ignoreUnused(midiMessages);

    const int oversampleAmount = (hq != nullptr) ? hq->get() : 0;
    {
        dryWetMixer.setWetLatency(oversamplingStack.getLatencySamples());

        oversamplingStack.setOversamplingFactor(oversampleAmount);
        if (oldOversamplingFactor != oversampleAmount)
        {
            DBG("Oversampling changed to " << oversampleAmount);
            oldOversamplingFactor = oversampleAmount;
            prepareToPlay(getSampleRate(), buffer.getNumSamples());
        }
    }

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    if (totalNumInputChannels == 0)
        return;
    if (totalNumOutputChannels == 0)
        return;

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    const auto gainAmount = (inputGainKnob != nullptr) ? inputGainKnob->get() : 0.0f;
    inputGain.setGainDecibels(gainAmount);
    inputGain.process(context);

    dryWetMixer.pushDrySamples(block);

    juce::dsp::AudioBlock<float> oversampledBlock = oversamplingStack.processSamplesUp(block);

    emphasisFilter.beforeProcessing(oversampledBlock.getNumSamples());

    const auto routingOrder = unpackRouting (order.load (std::memory_order_acquire));
    const auto scopeFocus = scopeContext.getFocus();
    for (size_t i = 0; i < (size_t) ModuleId::count; i++) {
        const auto moduleId = routingOrder[i];
        EffectBase* slot = slots[(size_t)moduleId];

        slot->updateParamsEveryBlock();

        const bool tapHere = moduleId == scopeFocus.module && (moduleId == ModuleId::preDistortion || moduleId == ModuleId::postDistortion);

        if (tapHere) 
            scopeDataCollector.capturePreDistortion(oversampledBlock.getChannelPointer(0), oversampledBlock.getNumSamples(), oversampleAmount);

        if (auto* levels = slotLevels[(size_t) moduleId]) {
            levels->process(oversampledBlock, [&] { slot->processBlock(oversampledBlock); });
        }
        else {
            slot->processBlock(oversampledBlock);
        }

        if (tapHere)
            scopeDataCollector.capturePostDistortion(oversampledBlock.getChannelPointer(0), oversampledBlock.getNumSamples(), oversampleAmount);
    }


    oversamplingStack.processSamplesDown(block);

    scopeDataCollector.process(buffer.getReadPointer(0), buffer.getReadPointer(1), (size_t)buffer.getNumSamples());

    outputGain.setGainDecibels((outputGainKnob != nullptr) ? outputGainKnob->get() : 0.0f);
    outputGain.process(context);

    dryWetMixer.setWetMixProportion((mixKnob != nullptr) ? (mixKnob->get() * 0.01f) : 1.0f);

    dryWetMixer.mixWetSamples(block);
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor *AudioPluginAudioProcessor::createEditor()
{
    return new ResizableEditor(*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    std::unique_ptr<juce::XmlElement> xml(treeState.copyState().createXml());
    copyXmlToBinary(*xml, destData);
}

void AudioPluginAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() == nullptr)
        return;

    if (xmlState->hasTagName(treeState.state.getType()))
        Preset::loadState(treeState, juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
