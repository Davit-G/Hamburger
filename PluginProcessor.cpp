#include "PluginProcessor.h"
#include "./gui/PluginEditor.h"

#include <JuceHeader.h>
#include "gui/Modules/Panels/PanelNames.h"

using namespace juce;

#include <chrono>
#include <ctime>

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
                         ),
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
    MelatoninPerfetto::get().beginSession(300000);
#endif
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
#if PERFETTO
    MelatoninPerfetto::get().endSession();
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

    // params.add(std::make_unique<AudioParameterFloat>("emphasisLowFreq", "Emphasis Low Frequency", 30.0f, 200.0f, 62.f));
    params.add(std::make_unique<AudioParameterFloat>("emphasisLowFreq", "Emphasis Low Frequency", NormalisableRange<float>(30.0f, 200.0f, 0.f, 0.25f), 62.0f));
    params.add(std::make_unique<AudioParameterFloat>("emphasisMidFreq", "Emphasis Mid Frequency", NormalisableRange<float>(500.0f, 3000.0f, 0.f, 0.25f), 1220.0f));
    params.add(std::make_unique<AudioParameterFloat>("emphasisHighFreq", "Emphasis Hi Frequency", NormalisableRange<float>(6000.0f, 18000.0f, 0.f, 0.25f), 9000.0f));

    // params.add(std::make_unique<AudioParameterFloat>("frequencyShiftFreq", "Frequency Shift Amount", -500.0f, 500.0f, 0.0f));

    params.add(std::make_unique<AudioParameterBool>("compandingOn", "Compander On", false));
    params.add(std::make_unique<AudioParameterBool>("compressionOn", "Compressor On", true));
    params.add(std::make_unique<AudioParameterBool>("expansionOn", "Expander On", false));
    params.add(std::make_unique<AudioParameterBool>("emphasisOn", "Emphasis EQ On", true));
    params.add(std::make_unique<AudioParameterBool>("shifterOn", "Emphasis Shifter On", true));

    params.add(std::make_unique<AudioParameterBool>("preDistortionEnabled", "PreDistortion Enabled", true));
    params.add(std::make_unique<AudioParameterBool>("primaryDistortionEnabled", "Distortion Enabled", true));
    params.add(std::make_unique<AudioParameterBool>("noiseDistortionEnabled", "Noise Enabled", true));

    params.add(std::make_unique<AudioParameterBool>("hamburgerEnabled", "Enabled (Bypass)", true));
    params.add(std::make_unique<AudioParameterBool>("autoGain", "Auto Gain", false));

    params.add(std::make_unique<AudioParameterChoice>("oversamplingFactor", "Oversampling Factor", params::oversamplingFactor.categories, 0));

    params.add(std::make_unique<AudioParameterFloat>("compAttack", "Compander Attack", 0.0f, 200.0f, 150.f));
    params.add(std::make_unique<AudioParameterFloat>("compRelease", "Compander Release", 0.0f, 500.0f, 200.f));
    params.add(std::make_unique<AudioParameterFloat>("compThreshold", "Compander Threshold", -48.0f, 0.0f, -35.f));
    params.add(std::make_unique<AudioParameterFloat>("compRatio", "Compander Ratio", 1.0f, 10.0f, 3.5f));
    params.add(std::make_unique<AudioParameterFloat>("compOut", "Compander Makeup", -24.0f, 24.0f, 0.f));

    params.add(std::make_unique<AudioParameterChoice>("primaryDistortionType", "Distortion Type", params::distortion.categories, 0));
    params.add(std::make_unique<AudioParameterChoice>("preDistortionType", "Pre-Distortion Type", params::preDistortionTypes.categories, 0));
    params.add(std::make_unique<AudioParameterChoice>("noiseDistortionType", "Noise Type", params::noiseTypes.categories, 0));
    params.add(std::make_unique<AudioParameterChoice>("compressionType", "Compression Type", params::companderInfo.categories, 0));

    params.add(std::make_unique<AudioParameterFloat>("fuzz", "Fuzz", 0.0f, 100.0f, 0.0f));
    params.add(std::make_unique<AudioParameterFloat>("fuzzShape", "Fuzz Shape", 0.0f, 100.0f, 50.0f));
    params.add(std::make_unique<AudioParameterFloat>("sizzle", "Sizzle", 0.0f, 100.0f, 0.0f));
    params.add(std::make_unique<AudioParameterFloat>("fold", "Fold", 0.0f, 100.0f, 0.0f));
    params.add(std::make_unique<AudioParameterFloat>("bias", "Bias", -1.0f, 1.0f, 0.0f));

    params.add(std::make_unique<AudioParameterFloat>("grungeAmt", "Grunge Amount", 0.0f, 1.0f, 0.0f));
    params.add(std::make_unique<AudioParameterFloat>("grungeTone", "Grunge Tone", 0.0f, 1.0f, 0.5f));

    // using new layout system

    // primary distortions
    params.add(std::make_unique<AudioParameterFloat>("saturationAmount", "Saturation", 0.0f, 100.0f, 0.f));

    // noise distortions
    params.add(std::make_unique<AudioParameterFloat>("noiseAmount", "Noise Amount", 0.0f, 100.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>("noiseFrequency", "Noise Frequency", NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.25f), 4000.0f));
    params.add(std::make_unique<AudioParameterFloat>("noiseQ", "Noise Q", 0.1f, 3.0f, 0.7f));

    params.add(std::make_unique<AudioParameterFloat>("downsampleFreq", "Downsample Freq", NormalisableRange<float>(200.0f, 40000.0f, 0.f, 0.25f), 40000.0f));
    params.add(std::make_unique<AudioParameterFloat>("downsampleJitter", "Downsample Jitter", 0.0f, 1.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>("bitReduction", "Bit Reduction Amount", 1.0f, 32.0f, 32.f));

    params.add(std::make_unique<AudioParameterFloat>("tubeTone", "Tube Distortion Tone", -1.0f, 1.0f, -0.2f));

    params.add(std::make_unique<AudioParameterFloat>("reverbMix", "Reverb Amount", 0.0f, 100.0f, 100.f));
    params.add(std::make_unique<AudioParameterFloat>("reverbSize", "Reverb Size", 0.0f, 1.0f, 1.f));
    params.add(std::make_unique<AudioParameterFloat>("reverbWidth", "Reverb Width", 0.0f, 1.0f, 1.f));
    params.add(std::make_unique<AudioParameterFloat>("reverbDamping", "Reverb Damping", 0.0f, 1.0f, 0.3f));

    params.add(std::make_unique<AudioParameterFloat>("combDelay", "Comb Delay", 0.01f, 2000.0f, 0.01f));
    params.add(std::make_unique<AudioParameterFloat>("combFeedback", "Comb Feedback", -1.0f, 1.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>("combMix", "Comb Mix", 0.0f, 100.0f, 100.0f));

    params.add(std::make_unique<AudioParameterFloat>("allPassFreq", "AllPass Frequency", NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.25f), 400.0f));
    ;
    params.add(std::make_unique<AudioParameterFloat>("allPassQ", "AllPass Q", 0.01f, 1.41f, 0.1f));
    params.add(std::make_unique<AudioParameterFloat>("allPassAmount", "AllPass Number", 0.0f, 50.0f, 1.0f));

    return params;
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
              // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String AudioPluginAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // TRACE_DSP();
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
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
        // if (eqGainValue != prevEmphasis[i]) {    // TODOOOOOOOO
        *peakFilterBefore[i].state = *dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, filterFrequencies[i], 0.5f, 0);
        *peakFilterAfter[i].state = *dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, filterFrequencies[i], 0.5f, 0);
        // }
        prevEmphasis[i] = 0;
    }

    oversamplingStack.prepare(spec);

    float totalLatency = oversamplingStack.getLatencySamples();
    // DBG("Total Latency: " << totalLatency);
    setLatencySamples((int)std::ceil(totalLatency));

    preDistortionSelection.prepare(spec);
    distortionTypeSelection.prepare(spec);
    noiseDistortionSelection.prepare(spec);

    dynamics.prepare(spec);

    distortionTypeSelection.prepare(spec);
}

void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

        // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
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

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.

    if (hamburgerEnabledButton->get() == false)
    {
        // DBG("Hamburger is disabled");
        return;
    }

    TRACE_EVENT_BEGIN("dsp", "audio block from buffer");
    dsp::AudioBlock<float> block(buffer);
    TRACE_EVENT_END("dsp");
    // dry/wet

    // input gain
    // Some computation here
    auto gainAmount = inputGainKnob->get();
    inputGain.setGainDecibels(gainAmount);
    inputGain.process(juce::dsp::ProcessContextReplacing<float>(block));

    {
        TRACE_EVENT("dsp", "dry/wet push");
        dryWetMixer.pushDrySamples(block);
    }

    bool emphasisOn = enableEmphasis->get();
    if (emphasisOn)
    {
        TRACE_EVENT("dsp", "emphasis EQ before");
        // update coeffs IF WE NEED TO
        for (int i = 0; i < 3; i++)
        {
            float eqGainValue = emphasis[i]->get();
            float eqFreqValue = emphasisFreq[i]->get();

            // DBG("EQ GAIN VALUE: " << eqGainValue);
            double grabbedSampleRate = getSampleRate();
            // if (eqGainValue != prevEmphasis[i]) {    // TODO: only update if changed
            *peakFilterBefore[i].state = *dsp::IIR::Coefficients<float>::makePeakFilter(grabbedSampleRate, eqFreqValue, 0.5f, Decibels::decibelsToGain(-eqGainValue));
            *peakFilterAfter[i].state = *dsp::IIR::Coefficients<float>::makePeakFilter(grabbedSampleRate, eqFreqValue, 0.5f, Decibels::decibelsToGain(eqGainValue));
            // }
            prevEmphasis[i] = eqGainValue;
        }

        for (int i = 0; i < 3; i++)
        {
            peakFilterBefore[i].process(dsp::ProcessContextReplacing<float>(block));
        }
    }

    // companding
    {
        TRACE_EVENT("dsp", "companding");
        dynamics.processBlock(block);
        // TRACE_EVENT_END("dsp", "companding");
    }
    // oversampling
    // bool oversamplingOn = enableOversampling->get();
    // i'd like to believe that changing the oversampling type mid-calculation will not affect it,
    // as long as it doesnt happen after this line and before the line where the oversampling is processed down again

    int oversampleAmount = oversamplingFactor->getIndex();

    int newSamplingRate = getSampleRate() * pow(2, oversampleAmount);
    {
        TRACE_EVENT("dsp", "oversampling config");

        dryWetMixer.setWetLatency(oversamplingStack.getLatencySamples());

        oversamplingStack.setOversamplingFactor(oversampleAmount);
        if (oldOversamplingFactor != oversampleAmount)
        {
            // DBG("Oversampling changed to " << oversampleAmount);
            oldOversamplingFactor = oversampleAmount;
            setLatencySamples(oversamplingStack.getLatencySamples());
        }
    }

    {
        TRACE_EVENT("dsp", "noise distortion");
        noiseDistortionSelection.setSampleRate(getSampleRate());
        noiseDistortionSelection.processBlock(block); // TODO: make order changer thingy
    }

    dsp::AudioBlock<float> oversampledBlock = oversamplingStack.processSamplesUp(block);

    {
        TRACE_EVENT("dsp", "pre distortion");
        preDistortionSelection.setSampleRate(newSamplingRate);
        preDistortionSelection.processBlock(oversampledBlock);
    }

    {
        TRACE_EVENT("dsp", "primary distortion");
        distortionTypeSelection.setSampleRate(newSamplingRate);
        distortionTypeSelection.processBlock(oversampledBlock);
    }

    // oversampling
    oversamplingStack.processSamplesDown(block);

    // tone with filter
    // here goes the second emphasis EQ before the expander
    if (emphasisOn)
    {
        TRACE_EVENT("dsp", "emphasis EQ after");
        for (int i = 0; i < 3; i++)
        {
            peakFilterAfter[i].process(dsp::ProcessContextReplacing<float>(block));
        }
    }

    {
        TRACE_EVENT("dsp", "other");
        // emphasis compensated gain
        float eqCompensation = (prevEmphasis[0] + prevEmphasis[1] + prevEmphasis[2]) * 0.3333333f * 0.4f;
        emphasisCompensationGain.setGainDecibels(-eqCompensation);
        emphasisCompensationGain.process(juce::dsp::ProcessContextReplacing<float>(block));

        // where the expander used to be
        // rip tho :(
        outputGain.setGainDecibels(outputGainKnob->get());
        outputGain.process(juce::dsp::ProcessContextReplacing<float>(block));

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
