#include "PluginProcessor.h"
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
    juce::AudioProcessorValueTreeState::ParameterLayout params;

    std::cout << "Creating parameters..." << std::endl;

    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::inputGain.id, "Input Gain", makeRange(-24.0f, 24.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::outputGain.id, "Out Gain", makeRange(-24.0f, 24.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::mix.id, "Mix", makeRange(0.0f, 100.0f), 100.f));

    // grill
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::saturationAmount.id, "Grill Saturation", makeRange(0.0f, 100.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::diode.id, "Grill Diode", makeRange(0.0f, 100.0f), 0.0f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::fold.id, "Grill Fold", makeRange(0.0f, 100.0f), 0.0f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::grillBias.id, "Grill Bias", makeRange(0.0f, 1.0f), 0.0f));

    // tubejuce::
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::tubeAmount.id, "Tube Saturation", makeRange(0.0f, 100.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::jeffAmount.id, "Tube Jeff Amt", makeRange(0.0f, 100.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::tubeBias.id, "Tube Bias", makeRange(0.0f, 1.0f), 0.0f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::tubeTone.id, "Tube Tone", makeRange(0.0f, 1.0f), 1.0f));

    // phasejuce::
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::phaseAmount.id, "Phase Distortion", makeRange(0.0f, 100.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::phaseDistTone.id, "Phase Dist Tone", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.25f), 355.0f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::phaseDistStereo.id, "Phase Dist Stereo", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::phaseShift.id, "Phase Dist Shift", makeRange(-1.0f, 1.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::phaseRectify.id, "Phase Dist Rectify", makeRange(0.0f, 1.0f), 0.f));

    // rubidiumjuce::
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::rubidiumAmount.id, "Rubidium Saturation", makeRange(0.0f, 100.0f), 5.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::rubidiumMojo.id, "Rubidium Mojo", makeRange(0.0f, 100.0f), 5.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::rubidiumAsym.id, "Rubidium Asymmetry", makeRange(0.0f, 10.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::rubidiumTone.id, "Rubidium Tone", juce::NormalisableRange<float>(4.0f, 100.0f, 0.f, 0.5f), 5.0f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::rubidiumBias.id, "Rubidium Bias", makeRange(0.0f, 1.0f), 0.f));

    // tapejuce::
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::tapeDrive.id, "Tape Drive", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::tapeBias.id, "Tape Bias", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::tapeWidth.id, "Tape Age", makeRange(0.0f, 1.0f), 0.3f));

    // compressorjuce::
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::compSpeed.id, "Comp Speed", juce::NormalisableRange<float>(0.0f, 400.0f, 0.f, 0.25f), 100.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::compBandTilt.id, "Comp Band Tilt", makeRange(-20.0f, 20.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::compStereoLink.id, "Stereo Link", makeRange(0.0f, 100.0f), 100.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::compRatio.id, "Comp Ratio", makeRange(1.0f, 10.0f), 3.5f));

    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::stereoCompThreshold.id, "Stereo Comp Threshold", makeRange(-48.0f, 0.0f), -24.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::MBCompThreshold.id, "MB Comp Threshold", makeRange(-48.0f, 0.0f), -24.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::MSCompThreshold.id, "MS Comp Threshold", makeRange(-48.0f, 0.0f), -24.f));

    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::TypeAThreshold.id, "Type A Threshold", makeRange(-48.0f, 0.0f), -40.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::TypeARatio.id, "Type A Ratio", makeRange(1.0f, 4.0f), 2.0f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::TypeATilt.id, "Type A Tilt", makeRange(-20.0f, 20.0f), -2.f));

    // noise distortionsjuce::
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::sizzleAmount.id, "Sizzle Amt", makeRange(0.0f, 100.0f), 5.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::sizzleFrequency.id, "Sizzle Freq", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.25f), 4000.0f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::sizzleQ.id, "Sizzle Q", makeRange(0.1f, 1.5f), 1.f));

    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::erosionAmount.id, "Erosion Amt", makeRange(0.0f, 100.0f), 3.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::erosionFrequency.id, "Noise Freq", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.25f), 400.0f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::erosionQ.id, "Erosion Q", makeRange(0.1f, 1.5f), 1.f));

    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::gateAmt.id, "Gate Amt", makeRange(0.0f, 1.0f), 0.f));

    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::downsampleFreq.id, "Dwnsmpl Freq", juce::NormalisableRange<float>(200.0f, 40000.0f, 0.f, 0.25f), 40000.0f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::bitReduction.id, "Dwnsmpl Bits", makeRange(1.0f, 32.0f), 32.f));

    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::fizzAmount.id, "Fizz Amt", makeRange(0.0f, 100.0f), 5.f));

    // predistjuce::
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::allPassFreq.id, "AllPass Frequency", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.25f), 85.0f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::allPassQ.id, "AllPass Q", makeRange(0.01f, 1.41f), 0.4f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::allPassAmount.id, "AllPass Number", makeRange(0.0f, 50.0f), 10.0f));

    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::grungeAmt.id, "Grunge Amt", makeRange(0.0f, 1.0f), 0.0f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::grungeTone.id, "Grunge Tone", makeRange(0.0f, 1.0f), 0.5f));

    // emphasisjuce::
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::emphasisLowGain.id, "Emphasis Low Gain", makeRange(-18.0f, 18.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::emphasisHighGain.id, "Emphasis Hi Gain", makeRange(-18.0f, 18.0f), 0.f));

    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::emphasisLowFreq.id, "Emphasis Low Frequency", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.25f), 62.0f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::emphasisHighFreq.id, "Emphasis Hi Frequency", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.25f), 9000.0f));

    // togglesjuce::
    params.add(std::make_unique<juce::AudioParameterBool>(ParamIDs::hamburgerEnabled.id, "Hamburger Enabled", true));
    params.add(std::make_unique<juce::AudioParameterBool>(ParamIDs::emphasisOn.id, "Emphasis EQ On", true));

    params.add(std::make_unique<juce::AudioParameterInt>(ParamIDs::oversamplingFactor.id, "Oversampling Factor", 0, 2, 0));

    // utilityjuce::
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::postClipGain.id, "SoftClip Gain", makeRange(-18.0f, 18.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::postClipKnee.id, "SoftClip Knee", makeRange(0.0f, 4.0f), 0.5f));
    
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::alphaParam.id, "Strength", makeRange(0.0f, 1.0f), 1.0f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::slewSpeed.id, "Slew Tone", makeRange(0.0f, 1.0f), 0.5f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::directionality.id, "Slew Bias", makeRange(-1.0f, 1.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterInt>(ParamIDs::slewType.id, "Slew Type", 0, 2, 0));


    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::MBCompSpeed.id, "Multiband Comp Speed", juce::NormalisableRange<float>(0.0f, 400.0f, 0.f, 0.25f), 100.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::MSCompSpeed.id, "Mid Side Comp Speed", juce::NormalisableRange<float>(0.0f, 400.0f, 0.f, 0.25f), 100.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::TypeACompSpeed.id, "Type A Comp Speed", juce::NormalisableRange<float>(0.0f, 400.0f, 0.f, 0.25f), 100.f));


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
