#include "PluginProcessor.h"
#include "dsp/EffectBase.h"
#include "gui/ResizableEditor.h"

#include <atomic>
#include <chrono>
#include <ctime>

#include "dsp/MacroParam.h"
#include "dsp/EffectInfos.h"


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
                                                         distortionTypeSelection(treeState)
{
    treeState.state = juce::ValueTree("savedParams");

    slots[(size_t) ModuleId::preEmphasis] = &emphasisPreFilter;
    slots[(size_t) ModuleId::postEmphasis] = &emphasisPostFilter;
    slots[(size_t) ModuleId::dynamics] = &dynamics;
    slots[(size_t) ModuleId::module1] = &noiseDistortionSelection;
    slots[(size_t) ModuleId::module2] = &preDistortionSelection;
    slots[(size_t) ModuleId::main] = &distortionTypeSelection;
    slots[(size_t) ModuleId::postClip] = &postClip;

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

    // the clipper gates itself on its own slot toggle, see PostClip::processBlock

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
    juce::AudioProcessorValueTreeState::ParameterLayout params;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> collected;
    
    std::cout << "Creating parameters..." << std::endl;
    
    
    collected.push_back(std::make_unique<MacroParam>(ParamIDs::inputGain));
    collected.push_back(std::make_unique<MacroParam>(ParamIDs::outputGain));
    collected.push_back(std::make_unique<MacroParam>(ParamIDs::mix));
    collected.push_back(std::make_unique<juce::AudioParameterInt>(ParamIDs::oversamplingFactor.getParameterID(), "Oversampling Factor", 0, 2, 0));
    
    for (int i = 0; i < ParamIDs::numGlobalMacros; ++i)
        collected.push_back (std::make_unique<MacroParam> (*ParamIDs::globalMacros[(size_t) i]));
    
    collected.push_back(std::make_unique<juce::AudioParameterInt>(ParamIDs::slewType.getParameterID(), "Slew Type", 0, 2, 0));
    collected.push_back(std::make_unique<juce::AudioParameterBool>(ParamIDs::hamburgerEnabled.getParameterID(), "Hamburger Enabled", true));
    collected.push_back(std::make_unique<juce::AudioParameterBool>(ParamIDs::emphasisOn.getParameterID(), "Emphasis EQ On", true));


    // compressorjuce::
    // collected.push_back(std::make_unique<MacroParam>(ParamIDs::compSpeed.getParamID(), "Comp Speed", juce::NormalisableRange<float>(0.0f, 400.0f, 0.f, 0.25f), 100.f));
    // collected.push_back(std::make_unique<MacroParam>(ParamIDs::compBandTilt.getParamID(), "Comp Band Tilt", makeRange(-20.0f, 20.0f), 0.f));
    // collected.push_back(std::make_unique<MacroParam>(ParamIDs::compStereoLink.getParamID(), "Stereo Link", makeRange(0.0f, 100.0f), 100.f));
    // collected.push_back(std::make_unique<MacroParam>(ParamIDs::compRatio.getParamID(), "Comp Ratio", makeRange(1.0f, 10.0f), 3.5f));
    // collected.push_back(std::make_unique<MacroParam>(ParamIDs::compOut.getParamID(), "Comp Makeup", makeRange(-24.0f, 24.0f), 0.f));

    // collected.push_back(std::make_unique<MacroParam>(ParamIDs::MBCompSpeed.getParamID(), "Multiband Comp Speed", juce::NormalisableRange<float>(0.0f, 400.0f, 0.f, 0.25f), 100.f));
    // collected.push_back(std::make_unique<MacroParam>(ParamIDs::MSCompSpeed.getParamID(), "Mid Side Comp Speed", juce::NormalisableRange<float>(0.0f, 400.0f, 0.f, 0.25f), 100.f));
    
    // collected.push_back(std::make_unique<MacroParam>(ParamIDs::stereoCompThreshold.getParamID(), "Stereo Comp Threshold", makeRange(-48.0f, 0.0f), -24.f));
    // collected.push_back(std::make_unique<MacroParam>(ParamIDs::MBCompThreshold.getParamID(), "MB Comp Threshold", makeRange(-48.0f, 0.0f), -24.f));
    // collected.push_back(std::make_unique<MacroParam>(ParamIDs::MSCompThreshold.getParamID(), "MS Comp Threshold", makeRange(-48.0f, 0.0f), -24.f));
    
    // collected.push_back(std::make_unique<MacroParam>(ParamIDs::TypeAThreshold.getParamID(), "Type A Threshold", makeRange(-48.0f, 0.0f), -40.f));
    // collected.push_back(std::make_unique<MacroParam>(ParamIDs::TypeARatio.getParamID(), "Type A Ratio", makeRange(1.0f, 4.0f), 2.0f));
    // collected.push_back(std::make_unique<MacroParam>(ParamIDs::TypeATilt.getParamID(), "Type A Tilt", makeRange(-20.0f, 20.0f), -2.f));
    // collected.push_back(std::make_unique<MacroParam>(ParamIDs::TypeAOut.getParamID(), "Type A Out", makeRange(-24.0f, 24.0f), -12.0f));
    // collected.push_back(std::make_unique<MacroParam>(ParamIDs::TypeACompSpeed.getParamID(), "Type A Comp Speed", juce::NormalisableRange<float>(0.0f, 400.0f, 0.f, 0.25f), 100.f));

    // emphasisjuce::
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
                    slot.type(), slot.displayNamePrefix() + " TYPE", types->categories, 0));

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

    float totalLatency = oversamplingStack.getLatencySamples();

    for (size_t i = 0; i < (size_t) ModuleId::count; i++) {
        EffectBase* slot = slots[i];
        slot->prepare(oversampledSpec);
        totalLatency += (float) slot->getLatencySamples();
    }

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

        dryWetMixer.setWetLatency(oversamplingStack.getLatencySamples());


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

    dryWetMixer.pushDrySamples(block);

    juce::dsp::AudioBlock<float> oversampledBlock = oversamplingStack.processSamplesUp(block);

    emphasisFilter.beforeProcessing(oversampledBlock.getNumSamples());

    const auto routingOrder = order.load(std::memory_order_acquire);
    for (size_t i = 0; i < (size_t) ModuleId::count; i++) {
        const auto moduleId = routingOrder[i];
        EffectBase* slot = slots[(size_t)moduleId];

        /*  Tap either side of the main distortion so the scope's pre/post traces follow it
            wherever the routing order puts it, rather than a fixed point in the chain. */
        const bool isMainDistortion = (moduleId == ModuleId::main);

        if (isMainDistortion)
            scopeDataCollector.capturePreDistortion(oversampledBlock.getChannelPointer(0),
                                                   oversampledBlock.getNumSamples(),
                                                   oversampleAmount);

        slot->updateParamsEveryBlock();
        slot->processBlock(oversampledBlock);

        if (isMainDistortion)
            scopeDataCollector.capturePostDistortion(oversampledBlock.getChannelPointer(0),
                                                    oversampledBlock.getNumSamples(),
                                                    oversampleAmount);
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
        // treeState.replaceState(juce::ValueTree::fromXml(*xmlState));
        juce::ValueTree copyState = juce::ValueTree::fromXml(*xmlState);
        treeState.replaceState(copyState);
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
