#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <JuceHeader.h>

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
      compander(treeState),
      pattyDistortion(treeState),
      sizzleNoise(treeState),
      cookedDistortion(treeState),
      oversampling(getTotalNumOutputChannels(), 1, dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false, false),
      softClipper(treeState),
      dryWetMixer(30)
//    tubeDistortion(treeState)
{
    treeState.state = ValueTree("savedParams");

    inputGainKnob = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter("inputGain")); jassert(inputGainKnob); 
    outputGainKnob = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter("outputGain")); jassert(outputGainKnob); 
    mixKnob = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter("mix")); jassert(mixKnob); 

    hamburgerEnabledButton = dynamic_cast<juce::AudioParameterBool *>(treeState.getParameter("hamburgerEnabled")); jassert(hamburgerEnabledButton);

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

    saturation = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter("saturation")); jassert(saturation); 

    oversamplingFactor = dynamic_cast<juce::AudioParameterChoice *>(treeState.getParameter("oversamplingFactor")); jassert(oversamplingFactor);

    enableCompander = dynamic_cast<juce::AudioParameterBool *>(treeState.getParameter("compandingOn")); jassert(enableCompander); 
    enableEmphasis = dynamic_cast<juce::AudioParameterBool *>(treeState.getParameter("emphasisOn")); jassert(enableEmphasis); 
    enableCompressor = dynamic_cast<juce::AudioParameterBool *>(treeState.getParameter("compressionOn")); jassert(enableCompander); 
    enableExpander = dynamic_cast<juce::AudioParameterBool *>(treeState.getParameter("expansionOn")); jassert(enableEmphasis);


}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
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
    params.add(std::make_unique<AudioParameterFloat>("emphasisLowFreq", "Emphasis Low Frequency", 30.0f, 200.0f, 62.f));
    params.add(std::make_unique<AudioParameterFloat>("emphasisMidFreq", "Emphasis Mid Frequency", 500.0f, 3000.0f, 1220.f));
    params.add(std::make_unique<AudioParameterFloat>("emphasisHighFreq", "Emphasis Hi Frequency", 6000.0f, 18000.0f, 9000.f));

    params.add(std::make_unique<AudioParameterBool>("compandingOn", "Compander On", false));
    params.add(std::make_unique<AudioParameterBool>("compressionOn", "Compressor On", true));
    params.add(std::make_unique<AudioParameterBool>("expansionOn", "Expander On", false));
    params.add(std::make_unique<AudioParameterBool>("emphasisOn", "Emphasis EQ On", true));

    params.add(std::make_unique<AudioParameterBool>("hamburgerEnabled", "Enabled (Bypass)", true));
    params.add(std::make_unique<AudioParameterBool>("autoGain", "Auto Gain", false));

    params.add(std::make_unique<AudioParameterChoice>("oversamplingFactor", "Oversampling Factor", oversamplingFactorChoices, 2));

    params.add(std::make_unique<AudioParameterFloat>("compAttack", "Compander Attack", 3.0f, 200.0f, 150.f));
    params.add(std::make_unique<AudioParameterFloat>("compRelease", "Compander Release", 10.0f, 500.0f, 200.f));
    params.add(std::make_unique<AudioParameterFloat>("compThreshold", "Compander Threshold", -48.0f, 0.0f, -35.f));
    params.add(std::make_unique<AudioParameterFloat>("compRatio", "Compander Ratio", 1.0f, 10.0f, 3.5f));
    params.add(std::make_unique<AudioParameterFloat>("compOut", "Compander Makeup", 0.0f, 24.0f, 0.f));

    params.add(std::make_unique<AudioParameterFloat>("fuzz", "Fuzz", 0.0f, 100.0f, 0.0f));
    params.add(std::make_unique<AudioParameterFloat>("sizzle", "Sizzle", 0.0f, 100.0f, 0.0f));
    params.add(std::make_unique<AudioParameterFloat>("fold", "Fold", 0.0f, 100.0f, 0.0f));
    params.add(std::make_unique<AudioParameterFloat>("saturation", "Saturation", 0.0f, 100.0f, 0.f));

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
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::ignoreUnused(sampleRate, samplesPerBlock);

    oversampling.initProcessing(samplesPerBlock);
    oversampling.reset();

    float totalLatency = oversampling.getLatencyInSamples();
    DBG("Total Latency: " << totalLatency);
    setLatencySamples(std::ceil(totalLatency));

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
        // DBG("EQ GAIN VALUE: " << eqGainValue);

        // if (eqGainValue != prevEmphasis[i]) {    // TODOOOOOOOO
        *peakFilterBefore[i].state = *dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, filterFrequencies[i], 0.5f, 0);
        *peakFilterAfter[i].state = *dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, filterFrequencies[i], 0.5f, 0);
        // }
        prevEmphasis[i] = 0;
    }
    
    oversampledSampleRate = sampleRate * pow(2, oversampling.getOversamplingFactor());

    oversamplingStack.prepare(spec);

    sizzleNoise.prepareToPlay(oversampledSampleRate, samplesPerBlock);

    compander.prepareToPlay(oversampledSampleRate, samplesPerBlock);
    pattyDistortion.prepareToPlay(oversampledSampleRate, samplesPerBlock);
    cookedDistortion.prepareToPlay(oversampledSampleRate, samplesPerBlock);
    softClipper.prepareToPlay(oversampledSampleRate, samplesPerBlock);
    // tubeDistortion.prepareToPlay(sampleRate, samplesPerBlock);
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

    if (hamburgerEnabledButton->get() == false) {
        // DBG("Hamburger is disabled");
        return;
    }

    dsp::AudioBlock<float> block(buffer);

    // dry/wet
    
    dryWetMixer.pushDrySamples(block);

    // input gain
    // Some computation here
    auto gainAmount = inputGainKnob->get();
    inputGain.setGainDecibels(gainAmount);
    inputGain.process(juce::dsp::ProcessContextReplacing<float>(block));

    bool emphasisOn = enableEmphasis->get();
    if (emphasisOn)
    {
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
    bool companderOn = enableCompander->get();

    if (companderOn) compander.updateParameters();
    if (companderOn && enableCompressor->get()) compander.processCompressorBlock(buffer);


    // oversampling
    // bool oversamplingOn = enableOversampling->get();
    // i'd like to believe that changing the oversampling type mid-calculation will not affect it, 
    // as long as it doesnt happen after this line and before the line where the oversampling is processed down again
    dsp::AudioBlock<float> oversampledBlock = oversampling.processSamplesUp(block);

    sizzleNoise.processBlock(oversampledBlock);
    cookedDistortion.processBlock(oversampledBlock); // maybe before distortion could be interesting
    pattyDistortion.processBlock(oversampledBlock);
    softClipper.processBlock(oversampledBlock);
    // tubeDistortion.processBlock(buffer);     // here goes the different distortion types stuff etc
    

    // oversampling
    oversampling.processSamplesDown(block);

    // tone with filter
    // here goes the second emphasis EQ before the expander
    if (emphasisOn) {
        for (int i = 0; i < 3; i++)
        {
            peakFilterAfter[i].process(dsp::ProcessContextReplacing<float>(block));
        }
    }
    // emphasis compensated gain
    float eqCompensation = (prevEmphasis[0] + prevEmphasis[1] + prevEmphasis[2]) * 0.3333333f * 0.4f;
    emphasisCompensationGain.setGainDecibels(-eqCompensation);
    emphasisCompensationGain.process(juce::dsp::ProcessContextReplacing<float>(block));

    // expander at the end
    if (companderOn && enableExpander->get()) compander.processExpanderBlock(buffer);

    outputGain.setGainDecibels(outputGainKnob->get());
    outputGain.process(juce::dsp::ProcessContextReplacing<float>(block));

    dryWetMixer.setWetMixProportion(mixKnob->get() * 0.01f);

    dryWetMixer.mixWetSamples(block);

    // TODO: Limiter at the end to stop clipping 
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor(*this);
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
