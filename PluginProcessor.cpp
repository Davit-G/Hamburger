#include "PluginProcessor.h"
#include "dsp/EffectBase.h"
#include "gui/ResizableEditor.h"

#include <atomic>
#include <chrono>
#include <ctime>

#include "dsp/MacroParam.h"
#include "dsp/EffectInfos.h"
#include "utils/Params.h"


//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor() : AudioProcessor(BusesProperties()
                                                                            .withInput("Input", juce::AudioChannelSet::stereo(), true)
                                                                            .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
                                                         treeState(*this, nullptr, "PARAMETER", createParameterLayout()),
                                                         dynamics(treeState, scopeDataCollector),
                                                         postClip(treeState, scopeDataCollector),
                                                         dryWetMixer(30),
                                                         noiseDistortionSelection(treeState),
                                                         preDistortionSelection(treeState),
                                                         emphasisFilter(treeState),
                                                         distortionTypeSelection(treeState, scopeDataCollector),
                                                         preDistortion(treeState, SlotId {ModuleId::preDistortion, 0}),
                                                         postDistortion(treeState, SlotId {ModuleId::postDistortion, 0})
{
    treeState.state = juce::ValueTree("savedParams");
    treeState.state.setProperty(Preset::versionProperty, JucePlugin_VersionString, nullptr);

    slots[(size_t) ModuleId::preEmphasis] = &emphasisPreFilter;
    slots[(size_t) ModuleId::postEmphasis] = &emphasisPostFilter;
    slots[(size_t) ModuleId::dynamics] = &dynamics;
    slots[(size_t) ModuleId::module1] = &noiseDistortionSelection;
    slots[(size_t) ModuleId::module2] = &preDistortionSelection;
    slots[(size_t) ModuleId::main] = &distortionTypeSelection;
    slots[(size_t) ModuleId::postClip] = &postClip;
    slots[(size_t) ModuleId::preDistortion] = &preDistortion;
    slots[(size_t) ModuleId::postDistortion] = &postDistortion;

    inputGainKnob = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter(ParamIDs::inputGain.getParamID()));
    if (inputGainKnob == nullptr)
        jassertfalse;

    outputGainKnob = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter(ParamIDs::outputGain.getParamID()));
    gainLink = dynamic_cast<juce::AudioParameterBool *>(treeState.getParameter(ParamIDs::gainLink.getParamID()));
    jassert(gainLink);
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

    // the clipper gates itself on its own slot toggle, see PostClip::processBlock

    presetManager = std::make_unique<Preset::PresetManager>(treeState, appProperties);

    // replaceState() swaps the tree out from under its listeners, which is valueTreeRedirected
    treeState.state.addListener(this);

#if PERFETTO
    // MelatoninPerfetto::get().beginSession(300000);
#endif
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
    treeState.state.removeListener(this);

#if PERFETTO
    // MelatoninPerfetto::get().endSession();
#endif
}

void AudioPluginAudioProcessor::setRoutingOrder(const RoutingOrder& newOrder)
{
    treeState.state.setProperty(routingOrderProperty, (juce::int64) packRouting(newOrder), nullptr);
}

// anything that isn't every module exactly once - a state from before reordering existed, or a damaged one - is the default
void AudioPluginAudioProcessor::syncRoutingOrder()
{
    const auto stored = treeState.state.getProperty(routingOrderProperty);

    auto newOrder = defaultRouting;

    if (stored.isInt64() || stored.isInt())
    {
        const auto candidate = unpackRouting((juce::uint64) (juce::int64) stored);

        std::array<bool, (size_t) ModuleId::count> seen {};
        auto valid = true;

        for (auto id : candidate)
        {
            const auto index = (size_t) id;
            valid = valid && index < seen.size() && !seen[index];

            if (valid)
                seen[index] = true;
        }

        if (valid)
            newOrder = candidate;
    }

    order.store(packRouting(newOrder), std::memory_order_release);
}

void AudioPluginAudioProcessor::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    if (tree == treeState.state && property == routingOrderProperty)
        syncRoutingOrder();
}

void AudioPluginAudioProcessor::valueTreeRedirected(juce::ValueTree& tree)
{
    if (tree == treeState.state)
        syncRoutingOrder();
}

juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout params;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> collected;
    
    std::cout << "Creating parameters..." << std::endl;
    
    
    collected.push_back(std::make_unique<MacroParam>(ParamIDs::inputGain));
    collected.push_back(std::make_unique<MacroParam>(ParamIDs::outputGain));
    collected.push_back(std::make_unique<juce::AudioParameterBool>(ParamIDs::gainLink.getParameterID(), ParamIDs::gainLink.displayName, false));
    collected.push_back(std::make_unique<MacroParam>(ParamIDs::mix));
    collected.push_back(std::make_unique<juce::AudioParameterInt>(ParamIDs::oversamplingFactor.getParameterID(), "Oversampling Factor", 0, 2, 0));
    
    for (int i = 0; i < ParamIDs::numGlobalMacros; ++i)
        collected.push_back (std::make_unique<MacroParam> (*ParamIDs::globalMacros[(size_t) i]));
    
    collected.push_back(std::make_unique<juce::AudioParameterChoice>(ParamIDs::mainRouting.getParameterID(), "Routing", ParamIDs::withReservedSlots (ParamIDs::routingTypes.categories), 0));
    collected.push_back(std::make_unique<juce::AudioParameterInt>(ParamIDs::bandCount.getParameterID(), ParamIDs::bandCount.displayName,
                                                                  (int) ParamIDs::bandCount.range.start, (int) ParamIDs::bandCount.range.end,
                                                                  (int) ParamIDs::bandCount.defaultValue));
    collected.push_back(std::make_unique<juce::AudioParameterInt>(ParamIDs::stackCount.getParameterID(), "Stack Count", 1, MainRouting::maxSlots, (int) ParamIDs::stackCount.defaultValue));
    collected.push_back(std::make_unique<juce::AudioParameterBool>(ParamIDs::stackFlip.getParameterID(), "Stack Flip Polarity", true));
    collected.push_back(std::make_unique<juce::AudioParameterChoice>(ParamIDs::stackFilter.getParameterID(), ParamIDs::stackFilter.displayName, ParamIDs::withReservedSlots (ParamIDs::stackFilterTypes.categories), 0));

    collected.push_back(std::make_unique<MacroParam>(ParamIDs::crossoverLow));
    collected.push_back(std::make_unique<MacroParam>(ParamIDs::crossoverMid));
    collected.push_back(std::make_unique<MacroParam>(ParamIDs::crossoverHigh));
    collected.push_back(std::make_unique<MacroParam>(ParamIDs::stackGain));
    collected.push_back(std::make_unique<MacroParam>(ParamIDs::stackMix));
    collected.push_back(std::make_unique<MacroParam>(ParamIDs::stackFilterFreq));
    collected.push_back(std::make_unique<MacroParam>(ParamIDs::stackFilterQ));
    collected.push_back(std::make_unique<MacroParam>(ParamIDs::stackRotation));
    collected.push_back(std::make_unique<MacroParam>(ParamIDs::msBalance));

    for (int band = 0; band < ParamIDs::numBands; ++band)
    {
        const auto& mute = *ParamIDs::bandMutes[band];
        const auto& solo = *ParamIDs::bandSolos[band];

        collected.push_back(std::make_unique<juce::AudioParameterBool>(mute.getParameterID(), mute.displayName, false));
        collected.push_back(std::make_unique<juce::AudioParameterBool>(solo.getParameterID(), solo.displayName, false));
    }

    collected.push_back(std::make_unique<juce::AudioParameterInt>(ParamIDs::slewType.getParameterID(), "Slew Type", 0, 2, 0));
    collected.push_back(std::make_unique<juce::AudioParameterBool>(ParamIDs::hamburgerEnabled.getParameterID(), "Hamburger Enabled", true));
    collected.push_back(std::make_unique<juce::AudioParameterBool>(ParamIDs::emphasisOn.getParameterID(), "Emphasis EQ On", true));

    
    collected.push_back(std::make_unique<juce::AudioParameterChoice>(ParamIDs::emphasisType.getParameterID(), ParamIDs::emphasisType.displayName,
                                                                     ParamIDs::withReservedSlots (ParamIDs::emphasisTypes.categories), 0));
    collected.push_back(std::make_unique<MacroParam>(ParamIDs::emphasisTilt));
    collected.push_back(std::make_unique<MacroParam>(ParamIDs::emphasisLowGain));
    collected.push_back(std::make_unique<MacroParam>(ParamIDs::emphasisHighGain));

    collected.push_back(std::make_unique<MacroParam>(ParamIDs::emphasisLowFreq));
    collected.push_back(std::make_unique<MacroParam>(ParamIDs::emphasisHighFreq));


    for (const auto& m : pluginModules)
    {
        for (int sub = 0; sub < m.slotCount; ++sub)
        {
            const SlotId slot { m.id, sub };

            collected.push_back (std::make_unique<juce::AudioParameterBool> (
                slot.enabled(), slot.displayNamePrefix() + " ENABLED", m.enabledByDefault));

            if (const auto* types = ParamIDs::categoriesFor (m.id))
                collected.push_back (std::make_unique<juce::AudioParameterChoice> (
                    slot.type(), slot.displayNamePrefix() + " TYPE", ParamIDs::withReservedSlots (types->categories), 0));

            // every distortion slot's own in, dry/wet and out, whichever type it is running
            if (ParamIDs::categoriesFor (m.id) == &ParamIDs::distortionTypes)
            {
                for (const auto* level : { &ParamIDs::slotInGain, &ParamIDs::slotMix, &ParamIDs::slotOutGain })
                    collected.push_back (std::make_unique<MacroParam> (slot, *level));

                collected.push_back (std::make_unique<juce::AudioParameterBool> (
                    paramIdFor (slot, ParamIDs::gainLink), slot.displayNamePrefix() + " " + ParamIDs::gainLink.displayName, false));
            }

            juce::StringArray alreadyAdded;

            for (const auto* layout : EffectInfos::layoutsFor (m.id))
            {
                for (const auto& descriptor : layout->params)
                {
                    if (descriptor.id.isNull() || alreadyAdded.contains (descriptor.getParamID()))
                        continue;

                    alreadyAdded.add (descriptor.getParamID());
                    collected.push_back (std::make_unique<MacroParam> (slot, descriptor));
                }
            }
        }
    }

    std::stable_sort (collected.begin(), collected.end(),
                      [] (const auto& a, const auto& b) { return a->getVersionHint() < b->getVersionHint(); });

    // see VERSION HINTS in ParamIDs
    DBG ("parameters: " << (int) collected.size());

    if constexpr (ParamIDs::releasedParameterCount > 0)
    {
        const auto released = std::count_if (collected.begin(), collected.end(),
                                             [] (const auto& p) { return p->getVersionHint() <= ParamIDs::releasedVersionHint; });

        // a parameter added since the last release still has a released version hint (give it releasedVersionHint + 1),
        // or a released parameter was removed. either breaks Logic and GarageBand automation
        jassert (released == ParamIDs::releasedParameterCount);
        juce::ignoreUnused (released);
    }

    for (auto& parameter : collected)
        params.add (std::move (parameter));

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

    emphasisFilter.prepare(oversampledSpec);

    for (size_t i = 0; i < (size_t) ModuleId::count; i++)
        slots[i]->prepare(oversampledSpec);

    dryWetMixer.reset();
    dryWetMixer.prepare(spec);
    updateLatency();

    scopeDataCollector.prepare(spec);
}

void AudioPluginAudioProcessor::updateLatency()
{
    const auto oversampledRatio = std::pow(2.0f, (float) oversamplingStack.getOversamplingFactor());

    auto total = oversamplingStack.getLatencySamples();

    for (auto* slot : slots)
        total += (float) slot->getLatencySamples() / oversampledRatio;

    const auto rounded = (int) std::ceil(total);

    if (rounded != getLatencySamples())
        setLatencySamples(rounded);

    dryWetMixer.setWetLatency(total);
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

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    if (totalNumInputChannels == 0)
        return;
    if (totalNumOutputChannels == 0)
        return;

    

    const int oversampleAmount = (hq != nullptr) ? hq->get() : 0;
    {
        // TRACE_EVENT("dsp", "oversampling config");

        oversamplingStack.setOversamplingFactor(oversampleAmount);
        if (oldOversamplingFactor != oversampleAmount)
        {
            DBG("Oversampling changed to " << oversampleAmount);
            oldOversamplingFactor = oversampleAmount;
            prepareToPlay(getSampleRate(), buffer.getNumSamples());
        }
    }


    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    const auto gainAmount = (inputGainKnob != nullptr) ? inputGainKnob->get() : 0.0f;
    inputGain.setGainDecibels(gainAmount);
    inputGain.process(context);

    scopeDataCollector.captureInput(buffer.getReadPointer(0), (size_t)buffer.getNumSamples());

    dryWetMixer.pushDrySamples(block);

    juce::dsp::AudioBlock<float> oversampledBlock = oversamplingStack.processSamplesUp(block);

    emphasisFilter.beforeProcessing(oversampledBlock.getNumSamples());

    
    const auto scopeFocus = scopeContext.getFocus();
    distortionTypeSelection.setScopeTap(scopeFocus.module == ModuleId::main ? scopeFocus.sub : -1, oversampleAmount);

    const auto routingOrder = unpackRouting (order.load (std::memory_order_acquire));
    for (size_t i = 0; i < (size_t) ModuleId::count; i++) {
        const auto moduleId = routingOrder[i];
        EffectBase* slot = slots[(size_t)moduleId];

        const bool tapHere = moduleId == scopeFocus.module
                             && (moduleId == ModuleId::preDistortion || moduleId == ModuleId::postDistortion);

        if (tapHere)
            scopeDataCollector.capturePreDistortion(oversampledBlock.getChannelPointer(0),
                                                   oversampledBlock.getNumSamples(),
                                                   oversampleAmount);

        slot->updateParamsEveryBlock();
        slot->processBlock(oversampledBlock);

        if (tapHere)
            scopeDataCollector.capturePostDistortion(oversampledBlock.getChannelPointer(0),
                                                    oversampledBlock.getNumSamples(),
                                                    oversampleAmount);
    }

    oversamplingStack.processSamplesDown(block);

    scopeDataCollector.process(buffer.getReadPointer(0), buffer.getReadPointer(1), (size_t)buffer.getNumSamples());

    // linked, the output takes back whatever the input added
    const auto linkedInDb = (gainLink != nullptr && gainLink->get()) ? gainAmount : 0.0f;
    outputGain.setGainDecibels(((outputGainKnob != nullptr) ? outputGainKnob->get() : 0.0f) - linkedInDb);
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
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    std::unique_ptr<juce::XmlElement> xml(treeState.copyState().createXml());
    copyXmlToBinary(*xml, destData);
}

void AudioPluginAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() == nullptr)
        return;

    if (xmlState->hasTagName(treeState.state.getType())) {
        Preset::loadState(treeState, juce::ValueTree::fromXml(*xmlState));
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
