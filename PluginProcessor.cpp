#include "PluginProcessor.h"
#include "./gui/PluginEditor.h"

using namespace juce;

#include "gui/Modules/Panels/PanelNames.h"

#include <chrono>
#include <ctime>

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor() : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      treeState(*this, nullptr, "PARAMETER", createParameterLayout()),
      dynamics(treeState),
      dryWetMixer(30),
      distortionTypeSelection(treeState),
      noiseDistortionSelection(treeState),
      preDistortionSelection(treeState)
{
    treeState.state = ValueTree("savedParams");

    inputGainKnob = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter("inputGain"));
    jassert(inputGainKnob);
    outputGainKnob = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter("outputGain"));
    jassert(outputGainKnob);
    mixKnob = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter("mix"));
    jassert(mixKnob);

    hamburgerEnabledButton = dynamic_cast<juce::AudioParameterBool *>(treeState.getParameter("hamburgerEnabled"));
    jassert(hamburgerEnabledButton);

    emphasisLow = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter("emphasisLowGain"));
    emphasis[0] = emphasisLow;
    jassert(emphasisLow);
    emphasisMid = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter("emphasisMidGain"));
    emphasis[1] = emphasisMid;
    jassert(emphasisMid);
    emphasisHigh = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter("emphasisHighGain"));
    emphasis[2] = emphasisHigh;
    jassert(emphasisHigh);

    emphasisLowFreq = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter("emphasisLowFreq"));
    emphasisFreq[0] = emphasisLowFreq;
    jassert(emphasisLowFreq);
    emphasisMidFreq = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter("emphasisMidFreq"));
    emphasisFreq[1] = emphasisMidFreq;
    jassert(emphasisMidFreq);
    emphasisHighFreq = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter("emphasisHighFreq"));
    emphasisFreq[2] = emphasisHighFreq;
    jassert(emphasisHighFreq);

    // freqShiftFreq = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter("frequencyShiftFreq")); jassert(freqShiftFreq);

    oversamplingFactor = dynamic_cast<juce::AudioParameterChoice *>(treeState.getParameter("oversamplingFactor"));
    jassert(oversamplingFactor);

    enableEmphasis = dynamic_cast<juce::AudioParameterBool *>(treeState.getParameter("emphasisOn"));
    jassert(enableEmphasis);

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

    params.add(std::make_unique<AudioParameterFloat>("inputGain", "Input Gain", -24.0f, 24.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>("outputGain", "Out Gain", -24.0f, 24.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>("mix", "Mix", 0.0f, 100.0f, 100.f));

    params.add(std::make_unique<AudioParameterFloat>("emphasisLowGain", "Emphasis Low Gain", -18.0f, 18.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>("emphasisMidGain", "Emphasis Mid Gain", -18.0f, 18.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>("emphasisHighGain", "Emphasis Hi Gain", -18.0f, 18.0f, 0.f));

    params.add(std::make_unique<AudioParameterFloat>("emphasisLowFreq", "Emphasis Low Frequency", NormalisableRange<float>(10.0f, 200.0f, 0.f, 0.35f), 62.0f));
    params.add(std::make_unique<AudioParameterFloat>("emphasisMidFreq", "Emphasis Mid Frequency", NormalisableRange<float>(300.0f, 3000.0f, 0.f, 0.4f), 1220.0f));
    params.add(std::make_unique<AudioParameterFloat>("emphasisHighFreq", "Emphasis Hi Frequency", NormalisableRange<float>(5000.0f, 18000.0f, 0.f, 0.4f), 9000.0f));

    params.add(std::make_unique<AudioParameterBool>("compandingOn", "Compander On", false));
    params.add(std::make_unique<AudioParameterBool>("compressionOn", "Compressor On", true));
    params.add(std::make_unique<AudioParameterBool>("expansionOn", "Expander On", false));
    params.add(std::make_unique<AudioParameterBool>("emphasisOn", "Emphasis EQ On", true));
    params.add(std::make_unique<AudioParameterBool>("shifterOn", "Emphasis Shifter On", true));

    params.add(std::make_unique<AudioParameterBool>("preDistortionEnabled", "Pre-Dist Enabled", true));
    params.add(std::make_unique<AudioParameterBool>("primaryDistortionEnabled", "Dist Enabled", true));
    params.add(std::make_unique<AudioParameterBool>("noiseDistortionEnabled", "Noise Enabled", true));

    params.add(std::make_unique<AudioParameterBool>("hamburgerEnabled", "Enabled", true));
    params.add(std::make_unique<AudioParameterBool>("autoGain", "Auto Gain", false));

    params.add(std::make_unique<AudioParameterChoice>("oversamplingFactor", "Oversampling Factor", params::oversamplingFactor.categories, 0));
    params.add(std::make_unique<AudioParameterChoice>("qualityFactor", "Quality", params::quality.categories, 0));

    params.add(std::make_unique<AudioParameterFloat>("compSpeed", "Comp Speed", 0.0f, 500.0f, 100.f));
    params.add(std::make_unique<AudioParameterFloat>("compBandTilt", "Comp Band Tilt", -12.0f, 12.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>("compStereoLink", "Stereo Link", 0.0f, 100.0f, 100.f));
    params.add(std::make_unique<AudioParameterFloat>("compThreshold", "Comp Threshold", -48.0f, 0.0f, -35.f));
    params.add(std::make_unique<AudioParameterFloat>("compRatio", "Comp Ratio", 1.0f, 10.0f, 3.5f));
    params.add(std::make_unique<AudioParameterFloat>("compOut", "Comp Makeup", -24.0f, 24.0f, 0.f));

    params.add(std::make_unique<AudioParameterChoice>("primaryDistortionType", "Dist Type", params::distortion.categories, 0));
    params.add(std::make_unique<AudioParameterChoice>("noiseDistortionType", "Noise Type", params::noiseTypes.categories, 0));
    params.add(std::make_unique<AudioParameterChoice>("compressionType", "Comp Type", params::dynamics.categories, 0));

    params.add(std::make_unique<AudioParameterFloat>("fuzz", "Fuzz", 0.0f, 100.0f, 0.0f));
    params.add(std::make_unique<AudioParameterFloat>("fuzzShape", "Fuzz Shape", 0.0f, 100.0f, 50.0f));
    params.add(std::make_unique<AudioParameterFloat>("sizzle", "Sizzle", 0.0f, 100.0f, 0.0f));
    params.add(std::make_unique<AudioParameterFloat>("fold", "Fold", 0.0f, 100.0f, 0.0f));
    
    params.add(std::make_unique<AudioParameterFloat>("bias", "Bias", -1.0f, 1.0f, 0.0f));

    params.add(std::make_unique<AudioParameterFloat>("grungeAmt", "Grunge Amt", 0.0f, 1.0f, 0.0f));
    params.add(std::make_unique<AudioParameterFloat>("grungeTone", "Grunge Tone", 0.0f, 1.0f, 0.5f));

    // primary distortions
    params.add(std::make_unique<AudioParameterFloat>("saturationAmount", "Saturation", 0.0f, 100.0f, 0.f));

    params.add(std::make_unique<AudioParameterFloat>("phaseDistTone", "Phase Distortion Tone", NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.25f), 20000.0f));

    // noise distortions
    params.add(std::make_unique<AudioParameterFloat>("noiseAmount", "Noise Amt", 0.0f, 100.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>("noiseFrequency", "Noise Freq", NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.25f), 4000.0f));
    params.add(std::make_unique<AudioParameterFloat>("noiseQ", "Noise Q", 0.1f, 3.0f, 0.7f));

    params.add(std::make_unique<AudioParameterFloat>("downsampleFreq", "Dwnsmpl Freq", NormalisableRange<float>(200.0f, 40000.0f, 0.f, 0.25f), 40000.0f));
    params.add(std::make_unique<AudioParameterFloat>("downsampleJitter", "Dwnsmpl Jitter", 0.0f, 1.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>("bitReduction", "Dwnsmpl Bits", 1.0f, 32.0f, 32.f));

    params.add(std::make_unique<AudioParameterFloat>("tubeTone", "Tube Tone", -1.0f, 1.0f, 1.0f));

    params.add(std::make_unique<AudioParameterFloat>("allPassFreq", "AllPass Frequency", NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.25f), 400.0f));
    params.add(std::make_unique<AudioParameterFloat>("allPassQ", "AllPass Q", 0.01f, 1.41f, 0.1f));
    params.add(std::make_unique<AudioParameterFloat>("allPassAmount", "AllPass Number", 0.0f, 50.0f, 0.0f));

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
    dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    dryWetMixer.reset();
    dryWetMixer.prepare(spec);
    dryWetMixer.setWetLatency(getLatencySamples());

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
        *peakFilterBefore[i].state = dsp::IIR::ArrayCoefficients<float>::makePeakFilter(spec.sampleRate, filterFrequencies[i], 0.5f, 1.0f);
        *peakFilterAfter[i].state = dsp::IIR::ArrayCoefficients<float>::makePeakFilter(spec.sampleRate, filterFrequencies[i], 0.5f, 1.0f);
        prevEmphasis[i] = 0;
    }

    oversamplingStack.prepare(spec);

    // float totalLatency = oversamplingStack.getLatencySamples();
    // // DBG("Total Latency: " << totalLatency);
    // setLatencySamples((int)std::ceil(totalLatency));

    preDistortionSelection.prepare(spec);
    distortionTypeSelection.prepare(spec);
    noiseDistortionSelection.prepare(spec);

    dynamics.prepare(spec);
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

    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    if (hamburgerEnabledButton->get() == false)
        return;

    TRACE_EVENT_BEGIN("dsp", "audio block from buffer");
    dsp::AudioBlock<float> block(buffer);
    dsp::ProcessContextReplacing<float> context(block);
    TRACE_EVENT_END("dsp");
    // dry/wet

    // input gain
    // Some computation here
    auto gainAmount = inputGainKnob->get();
    inputGain.setGainDecibels(gainAmount);
    inputGain.process(context);

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
            
            double grabbedSampleRate = getSampleRate();
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
    // oversampling
    // bool oversamplingOn = enableOversampling->get();
    // i'd like to believe that changing the oversampling type mid-calculation will not affect it,
    // as long as it doesnt happen after this line and before the line where the oversampling is processed down again

    // int oversampleAmount = oversamplingFactor->getIndex();

    // int newSamplingRate = getSampleRate() * pow(2, oversampleAmount);
    {
        TRACE_EVENT("dsp", "oversampling config");

        // dryWetMixer.setWetLatency(oversamplingStack.getLatencySamples());

        // oversamplingStack.setOversamplingFactor(oversampleAmount);
        // if (oldOversamplingFactor != oversampleAmount)
        // {
        //     // DBG("Oversampling changed to " << oversampleAmount);
        //     oldOversamplingFactor = oversampleAmount;
        //     setLatencySamples(oversamplingStack.getLatencySamples());
        // }
    }

    {
        TRACE_EVENT("dsp", "noise distortion");
        noiseDistortionSelection.processBlock(block); // TODO: make order changer thingy
    }

    // dsp::AudioBlock<float> oversampledBlock = oversamplingStack.processSamplesUp(block);

    {
        TRACE_EVENT("dsp", "pre distortion");
        preDistortionSelection.processBlock(block);
    }

    {
        TRACE_EVENT("dsp", "primary distortion");
        distortionTypeSelection.processBlock(block);
    }

    // oversampling
    // oversamplingStack.processSamplesDown(block);

    // tone with filter
    // here goes the second emphasis EQ before the expander
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

        // where the expander used to be
        // rip tho :(
        outputGain.setGainDecibels(outputGainKnob->get());
        outputGain.process(context);

        dryWetMixer.setWetMixProportion(mixKnob->get() * 0.01f);

        dryWetMixer.mixWetSamples(block);

        scopeDataCollector.process (buffer.getReadPointer (0), buffer.getReadPointer (1), (size_t) buffer.getNumSamples());
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
    if (xmlState.get() != nullptr)
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
