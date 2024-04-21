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
      preDistortionSelection(treeState),
      emphasisHighFreqSmooth(treeState, ParamIDs::emphasisHighFreq),
      emphasisLowFreqSmooth(treeState, ParamIDs::emphasisLowFreq),
      emphasisHighSmooth(treeState, ParamIDs::emphasisHighGain),
      emphasisLowSmooth(treeState, ParamIDs::emphasisLowGain)
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

    clipEnabled = dynamic_cast<juce::AudioParameterBool *>(treeState.getParameter(ParamIDs::postClipEnabled.getParamID()));
    jassert(clipEnabled);

    enableEmphasis = dynamic_cast<juce::AudioParameterBool *>(treeState.getParameter(ParamIDs::emphasisOn.getParamID()));
    jassert(enableEmphasis);

    presetManager = std::make_unique<Preset::PresetManager>(treeState);

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
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::phaseAmount, "Phase Saturation", makeRange(0.0f, 100.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::phaseDistTone, "Phase Distortion Tone", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.25f), 355.0f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::phaseDistNormalise, "Phase Normalisation", makeRange(0.0f, 1.0f), 0.f));

    // rubidium
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::rubidiumAmount, "Rubidium Saturation", makeRange(0.0f, 100.0f), 5.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::rubidiumMojo, "Rubidium Mojo", makeRange(0.0f, 100.0f), 5.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::rubidiumAsym, "Rubidium Asymmetry", makeRange(0.0f, 10.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::rubidiumTone, "Rubidium Tone", juce::NormalisableRange<float>(4.0f, 100.0f, 0.f, 0.5f), 5.0f));
    
    // matrix
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix1, "Matrix #1", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix2, "Matrix #2", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix3, "Matrix #3", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix4, "Matrix #4", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix5, "Matrix #5", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix6, "Matrix #6", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix7, "Matrix #7", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix8, "Matrix #8", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<AudioParameterFloat>(ParamIDs::matrix9, "Matrix #9", makeRange(0.0f, 1.0f), 0.f));

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
    emphasisCompensationGain.prepare(spec);

    // Initialize the filter
    for (int channel = 0; channel < 2; channel++) {
        for (int i = 0; i < 2; i++)
        {
            *peakFilterBefore[i][channel].coefficients = juce::dsp::IIR::ArrayCoefficients<double>::makePeakFilter(spec.sampleRate, filterFrequencies[i], 0.5f, 1.0f);
            *peakFilterAfter[i][channel].coefficients = juce::dsp::IIR::ArrayCoefficients<double>::makePeakFilter(spec.sampleRate, filterFrequencies[i], 0.5f, 1.0f);
            
            peakFilterBefore[i][channel].prepare(spec);
            peakFilterAfter[i][channel].prepare(spec);
        }
    }

    emphasisLowSmooth.prepare(spec);
    emphasisHighSmooth.prepare(spec);
    emphasisLowFreqSmooth.prepare(spec);
    emphasisHighFreqSmooth.prepare(spec);

    emphasisLowBuffer.reserve(spec.maximumBlockSize);
    emphasisHighBuffer.reserve(spec.maximumBlockSize);
    emphasisLowFreqBuffer.reserve(spec.maximumBlockSize);
    emphasisHighFreqBuffer.reserve(spec.maximumBlockSize);

    for (int i = 0; i < spec.maximumBlockSize; i++)
    {
        emphasisLowBuffer.push_back(0.0f);
        emphasisHighBuffer.push_back(0.0f);
        emphasisLowFreqBuffer.push_back(0.0f);
        emphasisHighFreqBuffer.push_back(0.0f);
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

    float prevEmphasis[2] = {0.f, 0.f};
    float prevFrequencies[2] = {62.0f, 9000.0f};

    bool updateLowCoefficients = false;
    bool updateHighCoefficients = false;

    bool emphasisOn = enableEmphasis->get();
    
    emphasisLowSmooth.update();
    emphasisHighSmooth.update();
    emphasisLowFreqSmooth.update();
    emphasisHighFreqSmooth.update();
    
    
    if (emphasisOn)
    {
        TRACE_EVENT("dsp", "emphasis EQ before");


        for (int i = 0; i < block.getNumSamples(); i++) {
            emphasisLowBuffer[i] = emphasisLowSmooth.get();
            emphasisHighBuffer[i] = emphasisHighSmooth.get();
            emphasisLowFreqBuffer[i] = emphasisLowFreqSmooth.get();
            emphasisHighFreqBuffer[i] = emphasisHighFreqSmooth.get();
        }

        prevEmphasis[0] = emphasisLowBuffer[0];
        prevEmphasis[1] = emphasisHighBuffer[0];
        prevFrequencies[0] = emphasisLowFreqBuffer[0];
        prevFrequencies[1] = emphasisHighFreqBuffer[0];

        // figure out if coefficients need updating

        double sRate = getSampleRate();

        for (int channel = 0; channel < block.getNumChannels(); channel++)
        {
            for (int sample = 0; sample < block.getNumSamples(); sample++)
            {
                float nextEmphasisLow = emphasisLowBuffer[sample];
                float nextEmphasisHigh = emphasisHighBuffer[sample];
                float nextEmphasisLowFreq = emphasisLowFreqBuffer[sample];
                float nextEmphasisHighFreq = emphasisHighFreqBuffer[sample];
                
                switch (channel) {
                    case 0: {
                        auto highCoeffs = juce::dsp::IIR::ArrayCoefficients<double>::makePeakFilter(sRate, nextEmphasisHighFreq, 0.5f, Decibels::decibelsToGain(-nextEmphasisHigh));
                        auto lowCoeffs = juce::dsp::IIR::ArrayCoefficients<double>::makePeakFilter(sRate, nextEmphasisLowFreq, 0.5f, Decibels::decibelsToGain(-nextEmphasisLow));

                        *peakFilterBefore[1][channel].coefficients = highCoeffs;
                        *peakFilterBefore[0][channel].coefficients = lowCoeffs;

                        break;
                    }
                    case 1: {
                        peakFilterBefore[1][1].coefficients = peakFilterBefore[1][0].coefficients;
                        peakFilterBefore[0][1].coefficients = peakFilterBefore[0][0].coefficients;

                        break;
                    }
                }

                auto interm = peakFilterBefore[0][channel].processSample(block.getSample(channel, sample));
                block.setSample(channel, sample, peakFilterBefore[1][channel].processSample(interm));
            }
        }
    }

    peakFilterBefore[0][0].snapToZero();
    peakFilterBefore[1][0].snapToZero();
    peakFilterBefore[0][1].snapToZero();
    peakFilterBefore[1][1].snapToZero();
    peakFilterAfter[0][0].snapToZero();
    peakFilterAfter[1][0].snapToZero();
    peakFilterAfter[0][1].snapToZero();
    peakFilterAfter[1][1].snapToZero();

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
        double sRate = getSampleRate();

        for (int channel = 0; channel < block.getNumChannels(); channel++)
        {
            for (int sample = 0; sample < block.getNumSamples(); sample++)
            {
                float nextEmphasisLow = emphasisLowBuffer[sample];
                float nextEmphasisHigh = emphasisHighBuffer[sample];
                float nextEmphasisLowFreq = emphasisLowFreqBuffer[sample];
                float nextEmphasisHighFreq = emphasisHighFreqBuffer[sample];

                switch (channel) {
                    case 0: {
                        auto highCoeffs = juce::dsp::IIR::ArrayCoefficients<double>::makePeakFilter(sRate, nextEmphasisHighFreq, 0.5f, Decibels::decibelsToGain(nextEmphasisHigh));
                        auto lowCoeffs = juce::dsp::IIR::ArrayCoefficients<double>::makePeakFilter(sRate, nextEmphasisLowFreq, 0.5f, Decibels::decibelsToGain(nextEmphasisLow));

                        *peakFilterAfter[1][0].coefficients = highCoeffs;
                        *peakFilterAfter[0][0].coefficients = lowCoeffs;

                        break;
                    }
                    case 1: {
                        peakFilterAfter[1][1].coefficients = peakFilterAfter[1][0].coefficients;
                        peakFilterAfter[0][1].coefficients = peakFilterAfter[0][0].coefficients;

                        break;
                    }
                }

                auto interm = peakFilterAfter[0][channel].processSample(block.getSample(channel, sample));
                block.setSample(channel, sample, peakFilterAfter[1][channel].processSample(interm));
            }
        }
    }

    {
        TRACE_EVENT("dsp", "other");
        // emphasis compensated gain
        float eqCompensation = (emphasisLowSmooth.getRaw() + emphasisHighSmooth.getRaw()) * 0.133f;
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
