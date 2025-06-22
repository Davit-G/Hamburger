#include "PluginProcessor.h"
#include "gui/PluginEditor.h"

#include <chrono>
#include <ctime>


//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor() : AudioProcessor(BusesProperties()
                                                                            .withInput("Input", juce::AudioChannelSet::stereo(), true)
                                                                            .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
                                                         treeState(*this, nullptr, "PARAMETER", createParameterLayout()),
                                                         dynamics(treeState),
                                                         postClip(treeState),
                                                         dryWetMixer(30),
                                                         noiseDistortionSelection(treeState),
                                                         preDistortionSelection(treeState),
                                                         emphasisFilter(treeState)
{
    treeState.state = ValueTree("savedParams");

    inputGainKnob = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter(ParamIDs::inputGain.getParamID()));
    jassert(inputGainKnob);
    outputGainKnob = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter(ParamIDs::outputGain.getParamID()));
    jassert(outputGainKnob);
    mixKnob = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter(ParamIDs::mix.getParamID()));
    jassert(mixKnob);

    hamburgerEnabledButton = dynamic_cast<juce::AudioParameterBool *>(treeState.getParameter(ParamIDs::hamburgerEnabled.getParamID()));
    jassert(hamburgerEnabledButton);

    stages = dynamic_cast<juce::AudioParameterInt *>(treeState.getParameter(ParamIDs::stages.getParamID()));
    jassert(stages);

    hq = dynamic_cast<juce::AudioParameterInt *>(treeState.getParameter(ParamIDs::oversamplingFactor.getParamID()));
    jassert(hq);

    clipEnabled = dynamic_cast<juce::AudioParameterBool *>(treeState.getParameter(ParamIDs::postClipEnabled.getParamID()));
    jassert(clipEnabled);

    presetManager = std::make_unique<Preset::PresetManager>(treeState);

    for (int i = 0; i < 4; i++) {
        distortionTypeSelection.push_back(std::make_unique<PrimaryDistortion>(treeState));
    }

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

inline auto makeRange(float start, float end)
{
    return juce::NormalisableRange<float>(start, end, 0.001f);
}

AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout()
{
    AudioProcessorValueTreeState::ParameterLayout params;

    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::inputGain, "Input Gain", makeRange(-24.0f, 24.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::outputGain, "Out Gain", makeRange(-24.0f, 24.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::mix, "Mix", makeRange(0.0f, 100.0f), 100.f));

    // grill
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::saturationAmount, "Grill Saturation", makeRange(0.0f, 100.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::diode, "Grill Diode", makeRange(0.0f, 100.0f), 0.0f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::fold, "Grill Fold", makeRange(0.0f, 100.0f), 0.0f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::grillBias, "Grill Bias", makeRange(0.0f, 1.0f), 0.0f));

    // tube
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::tubeAmount, "Tube Saturation", makeRange(0.0f, 100.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::jeffAmount, "Tube Jeff Amt", makeRange(0.0f, 100.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::tubeBias, "Tube Bias", makeRange(0.0f, 1.0f), 0.0f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::tubeTone, "Tube Tone", makeRange(0.0f, 1.0f), 1.0f));

    // phase
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::phaseAmount, "Phase Distortion", makeRange(0.0f, 100.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::phaseDistTone, "Phase Dist Tone", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.25f), 355.0f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::phaseDistStereo, "Phase Dist Stereo", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::phaseShift, "Phase Dist Shift", makeRange(-1.0f, 1.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::phaseRectify, "Phase Dist Rectify", makeRange(0.0f, 1.0f), 0.f));

    // rubidium
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::rubidiumAmount, "Rubidium Saturation", makeRange(0.0f, 100.0f), 5.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::rubidiumMojo, "Rubidium Mojo", makeRange(0.0f, 100.0f), 5.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::rubidiumAsym, "Rubidium Asymmetry", makeRange(0.0f, 10.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::rubidiumTone, "Rubidium Tone", juce::NormalisableRange<float>(4.0f, 100.0f, 0.f, 0.5f), 5.0f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::rubidiumBias, "Rubidium Bias", makeRange(0.0f, 1.0f), 0.f));

    // matrix
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix1, "Matrix #1", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix2, "Matrix #2", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix3, "Matrix #3", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix4, "Matrix #4", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix5, "Matrix #5", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix6, "Matrix #6", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix7, "Matrix #7", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix8, "Matrix #8", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix9, "Matrix #9", makeRange(0.0f, 1.0f), 1.f));
    
    // tape
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::tapeDrive, "Tape Drive", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::tapeBias, "Tape Bias", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::tapeWidth, "Tape Age", makeRange(0.0f, 1.0f), 0.3f));

    // categorical
    params.add(std::make_unique<AudioParameterChoice>(ParamIDs::primaryDistortionType, "Distortion Type", ParamIDs::distortion.categories, 0));
    params.add(std::make_unique<AudioParameterChoice>(ParamIDs::noiseDistortionType, "Noise Type", ParamIDs::noiseTypes.categories, 0));
    params.add(std::make_unique<AudioParameterChoice>(ParamIDs::compressionType, "Compression Type", ParamIDs::dynamics.categories, 0));

    // compressor
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::compSpeed, "Comp Speed", juce::NormalisableRange<float>(0.0f, 400.0f, 0.f, 0.25f), 100.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::compBandTilt, "Comp Band Tilt", makeRange(-20.0f, 20.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::compStereoLink, "Stereo Link", makeRange(0.0f, 100.0f), 100.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::compRatio, "Comp Ratio", makeRange(1.0f, 10.0f), 3.5f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::compOut, "Comp Makeup", makeRange(-24.0f, 24.0f), 0.f));

    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::stereoCompThreshold, "Stereo Comp Threshold", makeRange(-48.0f, 0.0f), -24.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::MBCompThreshold, "MB Comp Threshold", makeRange(-48.0f, 0.0f), -24.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::MSCompThreshold, "MS Comp Threshold", makeRange(-48.0f, 0.0f), -24.f));

    // noise distortions
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::sizzleAmount, "Sizzle Amt", makeRange(0.0f, 100.0f), 5.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::sizzleFrequency, "Sizzle Freq", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.25f), 4000.0f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::sizzleQ, "Sizzle Q", makeRange(0.1f, 1.5f), 1.f));

    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::erosionAmount, "Erosion Amt", makeRange(0.0f, 100.0f), 3.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::erosionFrequency, "Noise Freq", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.25f), 400.0f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::erosionQ, "Erosion Q", makeRange(0.1f, 1.5f), 1.f));

    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::gateAmt, "Gate Amt", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::gateMix, "Gate Mix", makeRange(0.0f, 1.0f), 1.f));

    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::downsampleFreq, "Dwnsmpl Freq", juce::NormalisableRange<float>(200.0f, 40000.0f, 0.f, 0.25f), 40000.0f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::downsampleMix, "Dwnsmpl Mix", makeRange(0.0f, 1.0f), 1.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::bitReduction, "Dwnsmpl Bits", makeRange(1.0f, 32.0f), 32.f));

    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::fizzAmount, "Fizz Amt", makeRange(0.0f, 100.0f), 5.f));

    // predist
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::allPassFreq, "AllPass Frequency", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.25f), 85.0f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::allPassQ, "AllPass Q", makeRange(0.01f, 1.41f), 0.4f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::allPassAmount, "AllPass Number", makeRange(0.0f, 50.0f), 10.0f));

    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::grungeAmt, "Grunge Amt", makeRange(0.0f, 1.0f), 0.0f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::grungeTone, "Grunge Tone", makeRange(0.0f, 1.0f), 0.5f));

    // emphasis
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::emphasisLowGain, "Emphasis Low Gain", makeRange(-18.0f, 18.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::emphasisHighGain, "Emphasis Hi Gain", makeRange(-18.0f, 18.0f), 0.f));

    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::emphasisLowFreq, "Emphasis Low Frequency", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.25f), 62.0f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::emphasisHighFreq, "Emphasis Hi Frequency", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.25f), 9000.0f));

    // toggles
    params.add(std::make_unique<AudioParameterBool>(ParamIDs::hamburgerEnabled, "Hamburger Enabled", true));
    params.add(std::make_unique<AudioParameterBool>(ParamIDs::compressionOn, "Compressor On", false));
    params.add(std::make_unique<AudioParameterBool>(ParamIDs::primaryDistortionEnabled, "Dist Enabled", true));
    params.add(std::make_unique<AudioParameterBool>(ParamIDs::emphasisOn, "Emphasis EQ On", true));
    params.add(std::make_unique<AudioParameterBool>(ParamIDs::preDistortionEnabled, "Pre-Dist Enabled", false));
    params.add(std::make_unique<AudioParameterBool>(ParamIDs::noiseDistortionEnabled, "Noise Enabled", false));
    params.add(std::make_unique<AudioParameterBool>(ParamIDs::postClipEnabled, "SoftClip Enabled", true));

    params.add(std::make_unique<AudioParameterInt>(ParamIDs::oversamplingFactor, "Oversampling Factor", 0, 2, 0));
    params.add(std::make_unique<AudioParameterInt>(ParamIDs::stages, "Stages", 1, 4, 1));

    // utility
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::postClipGain, "SoftClip Gain", makeRange(-18.0f, 18.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::postClipKnee, "SoftClip Knee", makeRange(0.0f, 4.0f), 0.5f));

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
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    inputGain.prepare(spec);
    outputGain.prepare(spec);


    oversamplingStack.setOversamplingFactor(hq->get());
    oversamplingStack.prepare(spec);

    juce::dsp::ProcessSpec oversampledSpec;
    oversampledSpec.sampleRate = sampleRate * pow(2, oversamplingStack.getOversamplingFactor());
    oversampledSpec.maximumBlockSize = samplesPerBlock * pow(2, oversamplingStack.getOversamplingFactor());
    oversampledSpec.numChannels = getTotalNumOutputChannels();

    for (int i = 0; i < 4; i++) {
        distortionTypeSelection[i]->prepare(oversampledSpec);
    }

    emphasisFilter.prepare(oversampledSpec);
    postClip.prepare(oversampledSpec);
    preDistortionSelection.prepare(oversampledSpec);
    noiseDistortionSelection.prepare(oversampledSpec);
    dynamics.prepare(oversampledSpec);

    float totalLatency = oversamplingStack.getLatencySamples();

    DBG("Total Latency: " << totalLatency);

    setLatencySamples((int)std::ceil(totalLatency));

    dryWetMixer.reset();
    dryWetMixer.prepare(spec);
    dryWetMixer.setWetLatency(totalLatency);
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
    if (hamburgerEnabledButton->get() == false)
        return;

    juce::ignoreUnused(midiMessages);

    {
        // TRACE_EVENT("dsp", "oversampling config");

        dryWetMixer.setWetLatency(oversamplingStack.getLatencySamples());

        int oversampleAmount = hq->get();

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

    auto gainAmount = inputGainKnob->get();
    inputGain.setGainDecibels(gainAmount);
    inputGain.process(context);

    dryWetMixer.pushDrySamples(block);

    dsp::AudioBlock<float> oversampledBlock = oversamplingStack.processSamplesUp(block);

    emphasisFilter.processBefore(oversampledBlock);

    {
        // TRACE_EVENT("dsp", "companding");
        dynamics.processBlock(oversampledBlock);
    }

    {
        // TRACE_EVENT("dsp", "noise distortion");
        noiseDistortionSelection.processBlock(oversampledBlock);
    }

    {
        // TRACE_EVENT("dsp", "pre distortion");
        preDistortionSelection.processBlock(oversampledBlock);
    }

    {
        int stagesAmt = stages->get();

        // TRACE_EVENT("dsp", "primary distortion");
        for (int i = 0; i < stagesAmt; i++) {
            if (i != 0) {
                juce::FloatVectorOperations::multiply(oversampledBlock.getChannelPointer(0), oversampledBlock.getChannelPointer(0), -1.0f, oversampledBlock.getNumSamples());
                juce::FloatVectorOperations::multiply(oversampledBlock.getChannelPointer(1), oversampledBlock.getChannelPointer(1), -1.0f, oversampledBlock.getNumSamples());
            }
            distortionTypeSelection[i]->processBlock(oversampledBlock);
        }
        if (stagesAmt % 2 == 0) {
            // when it's even, we need to flip the phase around again one more time
            // to avoid phase cancellation during mixing
            juce::FloatVectorOperations::multiply(oversampledBlock.getChannelPointer(0), oversampledBlock.getChannelPointer(0), -1.0f, oversampledBlock.getNumSamples());
            juce::FloatVectorOperations::multiply(oversampledBlock.getChannelPointer(1), oversampledBlock.getChannelPointer(1), -1.0f, oversampledBlock.getNumSamples());
        }
    }

    emphasisFilter.processAfter(oversampledBlock);

    {
        // TRACE_EVENT("dsp", "other");
        if (clipEnabled->get())
        {
            postClip.processBlock(oversampledBlock);
        }

        oversamplingStack.processSamplesDown(block);

        scopeDataCollector.process(buffer.getReadPointer(0), buffer.getReadPointer(1), (size_t)buffer.getNumSamples());

        outputGain.setGainDecibels(outputGainKnob->get());
        outputGain.process(context);

        dryWetMixer.setWetMixProportion(mixKnob->get() * 0.01f);

        dryWetMixer.mixWetSamples(block);
    }
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor *AudioPluginAudioProcessor::createEditor()
{
    return new EditorV2(*this);
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

    if (xmlState->hasTagName(treeState.state.getType()))
        // treeState.replaceState(juce::ValueTree::fromXml(*xmlState));
        treeState.state = ValueTree::fromXml(*xmlState);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
