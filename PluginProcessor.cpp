#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <JuceHeader.h>



using namespace juce;

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), treeState(*this, nullptr, "PARAMETER", createParameterLayout()), 
                       compressor(treeState, CompressionType::Compression, 2.5f, -30.f, 0.f, 25.f), // 3.5f, -20f, 0.3f
                       expander(treeState, CompressionType::Expansion, 3.5f, 0.f, 0.f, 0.f), // 3.5f, -20f, 0.3f
                       pattyDistortion(treeState),
                       sizzleNoise(treeState),
                       cookedDistortion(treeState),
                       compressorEnd(treeState, CompressionType::Compression, 200.f, -4.f, 3.f)
                    //    tubeDistortion(treeState)
{
    treeState.state = ValueTree("savedParams");

    inputGainKnob = dynamic_cast<juce::AudioParameterFloat*>(treeState.getParameter("inputGain"));
    jassert(inputGainKnob); // heads up if the parameter doesn't exist

    emphasisLow = dynamic_cast<juce::AudioParameterFloat*>(treeState.getParameter("emphasisLowGain"));
    emphasis[0] = emphasisLow;
    jassert(emphasisLow); // heads up if the parameter doesn't exist

    emphasisMid = dynamic_cast<juce::AudioParameterFloat*>(treeState.getParameter("emphasisMidGain"));
    emphasis[1] = emphasisMid;
    jassert(emphasisMid); // heads up if the parameter doesn't exist

    emphasisHigh = dynamic_cast<juce::AudioParameterFloat*>(treeState.getParameter("emphasisHighGain"));
    emphasis[2] = emphasisHigh;
    jassert(emphasisHigh); // heads up if the parameter doesn't exist



    saturation = dynamic_cast<juce::AudioParameterFloat*>(treeState.getParameter("saturation"));
    jassert(saturation); // heads up if the parameter doesn't exist

    enableCompander = dynamic_cast<juce::AudioParameterBool*>(treeState.getParameter("compOn"));
    jassert(enableCompander); // heads up if the parameter doesn't exist

    enableEmphasis = dynamic_cast<juce::AudioParameterBool*>(treeState.getParameter("emphasisOn"));
    jassert(enableEmphasis); // heads up if the parameter doesn't exist
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout() {
	AudioProcessorValueTreeState::ParameterLayout params;
	
    params.add(std::make_unique<AudioParameterFloat>("inputGain", "Input Gain", -24.0f, 24.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>("outputGain", "Out Gain", -24.0f, 24.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>("mix", "Mix", 0.0f, 100.0f, 100.f));

    params.add(std::make_unique<AudioParameterFloat>("emphasisLowGain", "Emphasis Low Gain", -18.0f, 18.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>("emphasisMidGain", "Emphasis Mid Gain", -18.0f, 18.0f, 0.f));
    params.add(std::make_unique<AudioParameterFloat>("emphasisHighGain", "Emphasis Hi Gain", -18.0f, 18.0f, 0.f));

    params.add(std::make_unique<AudioParameterBool>("compOn", "Compander On", false));
    params.add(std::make_unique<AudioParameterBool>("emphasisOn", "Emphasis EQ On", false));

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
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::ignoreUnused (sampleRate, samplesPerBlock);

    compressor.prepareToPlay(sampleRate, samplesPerBlock);
    expander.prepareToPlay(sampleRate, samplesPerBlock);
    pattyDistortion.prepareToPlay(sampleRate, samplesPerBlock);
    sizzleNoise.prepareToPlay(sampleRate, samplesPerBlock);
    cookedDistortion.prepareToPlay(sampleRate, samplesPerBlock);
    compressorEnd.prepareToPlay(sampleRate, samplesPerBlock);
    // tubeDistortion.prepareToPlay(sampleRate, samplesPerBlock);

    dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    // Initialize the filter
    for (int i = 0; i < 3; i++) {
        peakFilterBefore[i].prepare(spec);
        peakFilterAfter[i].prepare(spec);
    }

    cachedSampleRate = sampleRate;


    
}

void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.

    

    // update coeffs
    for (int i = 0; i < 3; i++) {
        *peakFilterBefore[i].state = *dsp::IIR::Coefficients<float>::makePeakFilter(cachedSampleRate, filterFrequencies[i], 0.807f,  Decibels::decibelsToGain( -emphasis[i]->get()));
        *peakFilterAfter[i].state = *dsp::IIR::Coefficients<float>::makePeakFilter(cachedSampleRate, filterFrequencies[i], 0.807f, Decibels::decibelsToGain( emphasis[i]->get()));
    }
    
    
    auto gainAmount = juce::Decibels::decibelsToGain(inputGainKnob->get());
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        for (int sample = 0; sample < buffer.getNumSamples(); sample++) {
            channelData[sample] *= gainAmount;
        }
    }

    // tone with filter
    
    dsp::AudioBlock<float> block(buffer);

    bool emphasisOn = enableEmphasis->get();
    if (emphasisOn) {
        for (int i = 0; i < 3; i++) {
            peakFilterBefore[i].process(dsp::ProcessContextReplacing<float>(block));
        }
    }

    bool companderOn = enableCompander->get();
    if (companderOn) compressor.processBlock(buffer);

    

    sizzleNoise.processBlock(buffer);

    cookedDistortion.processBlock(buffer); // maybe before distortion could be interesting
    pattyDistortion.processBlock(buffer);

    // tubeDistortion.processBlock(buffer);
    
	for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        

        for (int sample = 0; sample < buffer.getNumSamples(); sample++) {
            float xn = channelData[sample];
            // float saturation = 1.3;
            channelData[sample] = ((xn > 0) - (xn < 0))*(1.0 - exp(-fabs((saturation->get()*0.1f + 1.f)*xn))); // multiply audio on both channels by gain
        }
    }



    // tone with filter

    dsp::AudioBlock<float> block2(buffer);
    



    // here goes the second emphasis EQ before the expander
    if (emphasisOn) {
        for (int i = 0; i < 3; i++) {
            peakFilterAfter[i].process(dsp::ProcessContextReplacing<float>(block2));
        }
    }

    if (companderOn) expander.processBlock(buffer);
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    std::unique_ptr<juce::XmlElement> xml(treeState.copyState().createXml());
	copyXmlToBinary(*xml, destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
	if (xmlState.get() != nullptr)
		if (xmlState->hasTagName(treeState.state.getType()))
			//treeState.replaceState(juce::ValueTree::fromXml(*xmlState));
			treeState.state = ValueTree::fromXml(*xmlState);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
