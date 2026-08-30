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
                                                         noiseDistortionSelection(treeState),
                                                         preDistortionSelection(treeState),
                                                         emphasisFilter(treeState)
{
    treeState.state = juce::ValueTree("savedParams");

    inputGainKnob = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter(ParamIDs::inputGain.id.getParamID()));
    if (inputGainKnob == nullptr)
        jassertfalse;

    outputGainKnob = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter(ParamIDs::outputGain.id.getParamID()));
    if (outputGainKnob == nullptr)
        jassertfalse;

    mixKnob = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter(ParamIDs::mix.id.getParamID()));
    if (mixKnob == nullptr)
        jassertfalse;

    hamburgerEnabledButton = dynamic_cast<juce::AudioParameterBool *>(treeState.getParameter(ParamIDs::hamburgerEnabled.id.getParamID()));
    if (hamburgerEnabledButton == nullptr)
        jassertfalse;

    stages = dynamic_cast<juce::AudioParameterInt *>(treeState.getParameter(ParamIDs::stages.id.getParamID()));
    if (stages == nullptr)
        jassertfalse;

    hq = dynamic_cast<juce::AudioParameterInt *>(treeState.getParameter(ParamIDs::oversamplingFactor.id.getParamID()));
    if (hq == nullptr)
        jassertfalse;

    clipEnabled = dynamic_cast<juce::AudioParameterBool *>(treeState.getParameter(ParamIDs::postClipEnabled.id.getParamID()));
    if (clipEnabled == nullptr)
        jassertfalse;

    presetManager = std::make_unique<Preset::PresetManager>(treeState, appProperties);

    for (int i = 0; i < ParamIDs::maxStages; i++) {
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

    // matrixjuce::
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::matrix1.id, "Matrix #1", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::matrix2.id, "Matrix #2", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::matrix3.id, "Matrix #3", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::matrix4.id, "Matrix #4", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::matrix5.id, "Matrix #5", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::matrix6.id, "Matrix #6", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::matrix7.id, "Matrix #7", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::matrix8.id, "Matrix #8", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::matrix9.id, "Matrix #9", makeRange(0.0f, 1.0f), 1.f));

    // tapejuce::
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::tapeDrive.id, "Tape Drive", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::tapeBias.id, "Tape Bias", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::tapeWidth.id, "Tape Age", makeRange(0.0f, 1.0f), 0.3f));

    // categoricaljuce::
    params.add(std::make_unique<juce::AudioParameterChoice>(ParamIDs::primaryDistortionType.id, "Distortion Type", ParamIDs::distortion.categories, 0));
    params.add(std::make_unique<juce::AudioParameterChoice>(ParamIDs::noiseDistortionType.id, "Noise Type", ParamIDs::noiseTypes.categories, 0));
    params.add(std::make_unique<juce::AudioParameterChoice>(ParamIDs::compressionType.id, "Compression Type", ParamIDs::dynamics.categories, 0));

    // compressorjuce::
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::compSpeed.id, "Comp Speed", juce::NormalisableRange<float>(0.0f, 400.0f, 0.f, 0.25f), 100.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::compBandTilt.id, "Comp Band Tilt", makeRange(-20.0f, 20.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::compStereoLink.id, "Stereo Link", makeRange(0.0f, 100.0f), 100.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::compRatio.id, "Comp Ratio", makeRange(1.0f, 10.0f), 3.5f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::compOut.id, "Comp Makeup", makeRange(-24.0f, 24.0f), 0.f));

    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::stereoCompThreshold.id, "Stereo Comp Threshold", makeRange(-48.0f, 0.0f), -24.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::MBCompThreshold.id, "MB Comp Threshold", makeRange(-48.0f, 0.0f), -24.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::MSCompThreshold.id, "MS Comp Threshold", makeRange(-48.0f, 0.0f), -24.f));

    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::TypeAThreshold.id, "Type A Threshold", makeRange(-48.0f, 0.0f), -40.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::TypeARatio.id, "Type A Ratio", makeRange(1.0f, 4.0f), 2.0f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::TypeATilt.id, "Type A Tilt", makeRange(-20.0f, 20.0f), -2.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::TypeAOut.id, "Type A Out", makeRange(-24.0f, 24.0f), -12.0f));

    // noise distortionsjuce::
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::sizzleAmount.id, "Sizzle Amt", makeRange(0.0f, 100.0f), 5.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::sizzleFrequency.id, "Sizzle Freq", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.25f), 4000.0f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::sizzleQ.id, "Sizzle Q", makeRange(0.1f, 1.5f), 1.f));

    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::erosionAmount.id, "Erosion Amt", makeRange(0.0f, 100.0f), 3.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::erosionFrequency.id, "Noise Freq", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.f, 0.25f), 400.0f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::erosionQ.id, "Erosion Q", makeRange(0.1f, 1.5f), 1.f));

    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::gateAmt.id, "Gate Amt", makeRange(0.0f, 1.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::gateMix.id, "Gate Mix", makeRange(0.0f, 1.0f), 1.f));

    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::downsampleFreq.id, "Dwnsmpl Freq", juce::NormalisableRange<float>(200.0f, 40000.0f, 0.f, 0.25f), 40000.0f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::downsampleMix.id, "Dwnsmpl Mix", makeRange(0.0f, 1.0f), 1.f));
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
    params.add(std::make_unique<juce::AudioParameterBool>(ParamIDs::compressionOn.id, "Compressor On", false));
    params.add(std::make_unique<juce::AudioParameterBool>(ParamIDs::primaryDistortionEnabled.id, "Dist Enabled", true));
    params.add(std::make_unique<juce::AudioParameterBool>(ParamIDs::emphasisOn.id, "Emphasis EQ On", true));
    params.add(std::make_unique<juce::AudioParameterBool>(ParamIDs::preDistortionEnabled.id, "Pre-Dist Enabled", false));
    params.add(std::make_unique<juce::AudioParameterBool>(ParamIDs::noiseDistortionEnabled.id, "Noise Enabled", false));
    params.add(std::make_unique<juce::AudioParameterBool>(ParamIDs::postClipEnabled.id, "SoftClip Enabled", true));

    params.add(std::make_unique<juce::AudioParameterInt>(ParamIDs::oversamplingFactor.id, "Oversampling Factor", 0, 2, 0));
    params.add(std::make_unique<juce::AudioParameterInt>(ParamIDs::stages.id, "Stages", 1, ParamIDs::maxStages, 1));

    // utilityjuce::
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::postClipGain.id, "SoftClip Gain", makeRange(-18.0f, 18.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::postClipKnee.id, "SoftClip Knee", makeRange(0.0f, 4.0f), 0.5f));
    
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::alphaParam.id, "Strength", makeRange(0.0f, 1.0f), 1.0f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::slewSpeed.id, "Slew Tone", makeRange(0.0f, 1.0f), 0.5f));
    params.add(std::make_unique<juce::AudioParameterFloat>(ParamIDs::directionality.id, "Slew Bias", makeRange(-1.0f, 1.0f), 0.f));
    params.add(std::make_unique<juce::AudioParameterInt>(ParamIDs::slewType.id, "Slew Type", 0, 2, 0));
    
    params.add(std::make_unique<juce::AudioParameterChoice>(ParamIDs::preDistortionType.id, "Pre-Distortion Type", ParamIDs::preDistortionTypes.categories, 0));

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

    for (int i = 0; i < ParamIDs::maxStages; i++) {
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

    scopeDataCollector.capturePreDistortion(oversampledBlock.getChannelPointer(0), oversampledBlock.getNumSamples(), oversampleAmount);

    {
        const int stagesAmt = (stages != nullptr) ? stages->get() : 1;

        // TRACE_EVENT("dsp", "primary distortion");
        for (int i = 0; i < stagesAmt; i++) {
            if (i != 0) {
                float decreaseVolumeAmt = -0.7f; // negative includes the dc flip
                
                juce::FloatVectorOperations::multiply(oversampledBlock.getChannelPointer(0), oversampledBlock.getChannelPointer(0), decreaseVolumeAmt, oversampledBlock.getNumSamples());
                juce::FloatVectorOperations::multiply(oversampledBlock.getChannelPointer(1), oversampledBlock.getChannelPointer(1), decreaseVolumeAmt, oversampledBlock.getNumSamples());
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

    scopeDataCollector.capturePostDistortion(oversampledBlock.getChannelPointer(0), oversampledBlock.getNumSamples(), oversampleAmount);

    emphasisFilter.processAfter(oversampledBlock);

    {
        // TRACE_EVENT("dsp", "other");
        if (clipEnabled == nullptr || clipEnabled->get())
        {   
            postClip.processBlock(oversampledBlock);
        }

        oversamplingStack.processSamplesDown(block);

        scopeDataCollector.process(buffer.getReadPointer(0), buffer.getReadPointer(1), (size_t)buffer.getNumSamples());

        outputGain.setGainDecibels((outputGainKnob != nullptr) ? outputGainKnob->get() : 0.0f);
        outputGain.process(context);

        dryWetMixer.setWetMixProportion((mixKnob != nullptr) ? (mixKnob->get() * 0.01f) : 1.0f);

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

    if (xmlState->hasTagName(treeState.state.getType()))
        // treeState.replaceState(juce::ValueTree::fromXml(*xmlState));
        treeState.state = juce::ValueTree::fromXml(*xmlState);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
