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
      distortionTypeSelection(treeState),
      noiseDistortionSelection(treeState),
      preDistortionSelection(treeState)
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

    hq = dynamic_cast<juce::AudioParameterInt *>(treeState.getParameter(ParamIDs::oversamplingFactor.getParamID()));
    jassert(hq);

    emphasisLow = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter(ParamIDs::emphasisLowGain.getParamID()));
    emphasis[0] = emphasisLow;
    jassert(emphasisLow);
    emphasisMid = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter(ParamIDs::emphasisMidGain.getParamID()));
    emphasis[1] = emphasisMid;
    jassert(emphasisMid);
    emphasisHigh = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter(ParamIDs::emphasisHighGain.getParamID()));
    emphasis[2] = emphasisHigh;
    jassert(emphasisHigh);

    emphasisLowFreq = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter(ParamIDs::emphasisLowFreq.getParamID()));
    emphasisFreq[0] = emphasisLowFreq;
    jassert(emphasisLowFreq);
    emphasisMidFreq = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter(ParamIDs::emphasisMidFreq.getParamID()));
    emphasisFreq[1] = emphasisMidFreq;
    jassert(emphasisMidFreq);
    emphasisHighFreq = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter(ParamIDs::emphasisHighFreq.getParamID()));
    emphasisFreq[2] = emphasisHighFreq;
    jassert(emphasisHighFreq);

    clipEnabled = dynamic_cast<juce::AudioParameterBool *>(treeState.getParameter(ParamIDs::postClipEnabled.getParamID()));
    jassert(clipEnabled);

    enableEmphasis = dynamic_cast<juce::AudioParameterBool *>(treeState.getParameter(ParamIDs::emphasisOn.getParamID()));
    jassert(enableEmphasis);

    presetManager = std::make_unique<PresetManager>(treeState);

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

AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout()
{
    AudioProcessorValueTreeState::ParameterLayout params;

    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::inputGain, "Input Gain", -24.0f, 24.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::outputGain, "Out Gain", -24.0f, 24.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::mix, "Mix", 0.0f, 100.0f, 100.f));

    // grill
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::saturationAmount, "Grill Saturation", 0.0f, 100.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::diode, "Grill Diode", 0.0f, 100.0f, 0.0f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::fold, "Grill Fold", 0.0f, 100.0f, 0.0f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::grillBias, "Grill Bias", 0.0f, 1.0f, 0.0f));

    // tube
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::tubeAmount, "Tube Saturation", 0.0f, 100.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::jeffAmount, "Tube Jeff Amt", 0.0f, 100.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::tubeBias, "Tube Bias", 0.0f, 1.0f, 0.0f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::tubeTone, "Tube Tone", 0.0f, 1.0f, 1.0f));

    // phase
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::phaseAmount, "Phase Saturation", 0.0f, 100.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::phaseDistTone, "Phase Distortion Tone", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.25f), 355.0f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::phaseDistNormalise, "Phase Normalisation", 0.0f, 1.0f, 0.f));

    // rubidium
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::rubidiumAmount, "Rubidium Saturation", 0.0f, 100.0f, 5.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::rubidiumMojo, "Rubidium Mojo", 0.0f, 100.0f, 40.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::rubidiumAsym, "Rubidium Asymmetry", 0.0f, 10.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::rubidiumTone, "Rubidium Tone", juce::NormalisableRange<float>(4.0f, 100.0f, 0.f, 0.5f), 5.0f));
    
    // matrix
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix1, "Matrix #1", 0.0f, 1.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix2, "Matrix #2", 0.0f, 1.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix3, "Matrix #3", 0.0f, 1.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix4, "Matrix #4", 0.0f, 1.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix5, "Matrix #5", 0.0f, 1.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix6, "Matrix #6", 0.0f, 1.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix7, "Matrix #7", 0.0f, 1.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix8, "Matrix #8", 0.0f, 1.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix9, "Matrix #9", 0.0f, 1.0f, 0.f));

    // categorical
    params.add(std::make_unique<AudioParameterChoice>(ParamIDs::primaryDistortionType, "Distortion Type", ParamIDs::distortion.categories, 0));
    params.add(std::make_unique<AudioParameterChoice>(ParamIDs::noiseDistortionType, "Noise Type", ParamIDs::noiseTypes.categories, 0));
    params.add(std::make_unique<AudioParameterChoice>(ParamIDs::compressionType, "Compression Type", ParamIDs::dynamics.categories, 0));

    // compressor
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::compSpeed, "Comp Speed", juce::NormalisableRange<float>(0.0f, 400.0f, 0.f, 0.25f), 100.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::compBandTilt, "Comp Band Tilt", -20.0f, 20.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::compStereoLink, "Stereo Link", 0.0f, 100.0f, 100.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::compThreshold, "Comp Threshold", -48.0f, 0.0f, -24.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::compRatio, "Comp Ratio", 1.0f, 10.0f, 3.5f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::compOut, "Comp Makeup", -24.0f, 24.0f, 0.f));


    // noise distortions
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::sizzleAmount, "Sizzle Amt", 0.0f, 100.0f, 5.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::sizzleFrequency, "Sizzle Freq", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.25f), 4000.0f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::sizzleQ, "Sizzle Q", 0.1f, 1.5f, 1.f));

    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::erosionAmount, "Erosion Amt", 0.0f, 100.0f, 3.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::erosionFrequency, "Noise Freq", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.25f), 400.0f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::erosionQ, "Erosion Q", 0.1f, 1.5f, 1.f));

    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::gateAmt, "Gate Amt", 0.0f, 1.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::gateMix, "Gate Mix", 0.0f, 1.0f, 1.f));

    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::downsampleFreq, "Dwnsmpl Freq", juce::NormalisableRange<float>(200.0f, 40000.0f, 0.f, 0.25f), 40000.0f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::downsampleMix, "Dwnsmpl Mix", 0.0f, 1.0f, 1.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::bitReduction, "Dwnsmpl Bits", 1.0f, 32.0f, 32.f));

    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::allPassFreq, "AllPass Frequency", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.25f), 85.0f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::allPassQ, "AllPass Q", 0.01f, 1.41f, 0.4f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::allPassAmount, "AllPass Number", 0.0f, 50.0f, 10.0f));

    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::grungeAmt, "Grunge Amt", 0.0f, 1.0f, 0.0f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::grungeTone, "Grunge Tone", 0.0f, 1.0f, 0.5f));

    // emphasis
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::emphasisLowGain, "Emphasis Low Gain", -18.0f, 18.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::emphasisMidGain, "Emphasis Mid Gain", -18.0f, 18.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::emphasisHighGain, "Emphasis Hi Gain", -18.0f, 18.0f, 0.f));

    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::emphasisLowFreq, "Emphasis Low Frequency", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.25f), 62.0f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::emphasisMidFreq, "Emphasis Mid Frequency", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.25f), 1220.0f));
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

    // utility
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::postClipGain, "SoftClip Gain", -18.0f, 18.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::postClipKnee, "SoftClip Knee", 0.0f, 4.0f, 0.5f));

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
    emphasisCompensationGain.prepare(spec);

    // Initialize the filter
    for (int i = 0; i < 3; i++)
    {
        peakFilterBefore[i].prepare(spec);
        peakFilterAfter[i].prepare(spec);
    }

    // update coeffs IF WE NEED TO
    for (int i = 0; i < 3; i++)
    {
        *peakFilterBefore[i].state = juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter(spec.sampleRate, filterFrequencies[i], 0.5f, 1.0f);
        *peakFilterAfter[i].state = juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter(spec.sampleRate, filterFrequencies[i], 0.5f, 1.0f);
        prevEmphasis[i] = 0;
    }

    oversamplingStack.setOversamplingFactor(hq->get());
    oversamplingStack.prepare(spec);
    oversamplingStackPost.setOversamplingFactor(hq->get());
    oversamplingStackPost.prepare(spec);

    juce::dsp::ProcessSpec oversampledSpec;
    oversampledSpec.sampleRate = sampleRate * pow(2, oversamplingStack.getOversamplingFactor());
    oversampledSpec.maximumBlockSize = samplesPerBlock;
    oversampledSpec.numChannels = getTotalNumOutputChannels();

    distortionTypeSelection.prepare(oversampledSpec);
    postClip.prepare(oversampledSpec);

    preDistortionSelection.prepare(spec);
    noiseDistortionSelection.prepare(spec);

    dynamics.prepare(spec);

    float totalLatency = oversamplingStack.getLatencySamples() + oversamplingStackPost.getLatencySamples();
    DBG("Total Latency: " << totalLatency );
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
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
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

    // int newSamplingRate = getSampleRate() * pow(2, oversampleAmount);
    {
        TRACE_EVENT("dsp", "oversampling config");

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

    if (totalNumInputChannels == 0) return;
    if (totalNumOutputChannels == 0) return;
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    TRACE_EVENT_BEGIN("dsp", "audio block from buffer");

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    TRACE_EVENT_END("dsp");
    // dry/wet

    // input gain
    // Some computation here
    auto gainAmount = inputGainKnob->get();
    inputGain.setGainDecibels(gainAmount);
    inputGain.process(context);

    // xsimd::dispatch(SIMDGain::process{})(block, juce::Decibels::decibelsToGain(gainAmount));

    dryWetMixer.pushDrySamples(block);

    bool emphasisOn = enableEmphasis->get();
    if (emphasisOn)
    {
        TRACE_EVENT("dsp", "emphasis EQ before");
        // update coeffs IF WE NEED TO
        for (int i = 0; i < 3; i++)
        {
            float eqGainValue = emphasis[i]->get();
            float eqFreqValue = emphasisFreq[i]->get();
            
            float grabbedSampleRate = static_cast<float>(getSampleRate());
            if (eqGainValue != prevEmphasis[i] || eqFreqValue != prevEmphasisFreq[i])
            {
                *peakFilterBefore[i].state = dsp::IIR::ArrayCoefficients<float>::makePeakFilter(grabbedSampleRate, eqFreqValue, 0.5f, Decibels::decibelsToGain(-eqGainValue));
                *peakFilterAfter[i].state = dsp::IIR::ArrayCoefficients<float>::makePeakFilter(grabbedSampleRate, eqFreqValue, 0.5f, Decibels::decibelsToGain(eqGainValue));
                prevEmphasis[i] = eqGainValue;
                prevEmphasisFreq[i] = eqFreqValue;
            }
        }

        for (int i = 0; i < 3; i++)
        {
            peakFilterBefore[i].process(context);
        }
    }

    // companding
    {
        TRACE_EVENT("dsp", "companding");
        dynamics.processBlock(block);
    }

    {
        TRACE_EVENT("dsp", "noise distortion");
        noiseDistortionSelection.processBlock(block); // TODO: make order changer thingy
    }


    {
        TRACE_EVENT("dsp", "pre distortion");
        preDistortionSelection.processBlock(block);
    }

    dsp::AudioBlock<float> oversampledBlock = oversamplingStack.processSamplesUp(block);

    {
        TRACE_EVENT("dsp", "primary distortion");
        distortionTypeSelection.processBlock(oversampledBlock);
    }

    oversamplingStack.processSamplesDown(block);

    if (emphasisOn)
    {
        TRACE_EVENT("dsp", "emphasis EQ after");
        for (int i = 0; i < 3; i++)
        {
            peakFilterAfter[i].process(context);
        }
    }

    {
        TRACE_EVENT("dsp", "other");
        // emphasis compensated gain
        float eqCompensation = (prevEmphasis[0] + prevEmphasis[1] + prevEmphasis[2]) * 0.3333333f * 0.4f;
        emphasisCompensationGain.setGainDecibels(-eqCompensation);
        emphasisCompensationGain.process(context);

        dsp::AudioBlock<float> oversampledBlock2 = oversamplingStackPost.processSamplesUp(block);
        if (clipEnabled->get())
        {
            postClip.processBlock(oversampledBlock2);
        }
        oversamplingStackPost.processSamplesDown(block);


        // before output gain
        scopeDataCollector.process (buffer.getReadPointer (0), buffer.getReadPointer (1), (size_t) buffer.getNumSamples());
        
        outputGain.setGainDecibels(outputGainKnob->get());
        outputGain.process(context);

        dryWetMixer.setWetMixProportion(mixKnob->get() * 0.01f);

        dryWetMixer.mixWetSamples(block);

    }
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
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
    if (xmlState.get() == nullptr) return;

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
