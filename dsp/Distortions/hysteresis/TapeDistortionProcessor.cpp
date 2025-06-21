#include "TapeDistortionProcessor.h"

namespace
{
enum
{
    numSteps = 500,
};

template <typename T>
static void interleaveSamples (const T** source, T* dest, int numSamples, int numChannels)
{
    for (int chan = 0; chan < numChannels; ++chan)
    {
        auto i = chan;
        const auto* src = source[chan];

        for (int j = 0; j < numSamples; ++j)
        {
            dest[i] = src[j];
            i += numChannels;
        }
    }
}

template <typename T>
static void deinterleaveSamples (const T* source, T** dest, int numSamples, int numChannels)
{
    for (int chan = 0; chan < numChannels; ++chan)
    {
        auto i = chan;
        auto* dst = dest[chan];

        for (int j = 0; j < numSamples; ++j)
        {
            dst[j] = source[i];
            i += numChannels;
        }
    }
}

template <typename T>
constexpr T ceiling_divide (T num, T den)
{
    return (num + den - 1) / den;
}
} // namespace

void TapeDistortionProcessor::setParameter (Param param, float newValue)
{
    jassert (juce::isWithin (newValue, 0.0f, 1.0f)); // parameter value is outside the allowed range!

    if (param == Param::Drive)
    {
        setDrive (newValue);
    }
    else if (param == Param::Saturation)
    {
        setSaturation (newValue);
    }
    else if (param == Param::Bias)
    {
        setWidth (1.0f - newValue);
    }
}

double TapeDistortionProcessor::calcMakeup()
{
    return (1.0 + 0.6 * width[0].getTargetValue()) / (0.5 + 1.5 * (1.0 - sat[0].getTargetValue()));
}

void TapeDistortionProcessor::setDrive (float newDrive)
{
    for (auto& driveVal : drive)
        driveVal.setTargetValue ((double) newDrive);
}

void TapeDistortionProcessor::setWidth (float newWidth)
{
    for (auto& widthVal : width)
        widthVal.setTargetValue ((double) newWidth);
}

void TapeDistortionProcessor::setSaturation (float newSaturation)
{
    for (auto& satVal : sat)
        satVal.setTargetValue ((double) newSaturation);
}

void TapeDistortionProcessor::prepareToPlay (double sampleRate, int samplesPerBlock, int numChannels)
{
    oversampling = std::make_unique<juce::dsp::Oversampling<double>> (numChannels, 2, juce::dsp::Oversampling<double>::filterHalfBandPolyphaseIIR);
    oversampling->initProcessing ((size_t) samplesPerBlock);

    drive.resize ((size_t) numChannels);
    for (auto& val : drive)
        val.reset (numSteps);

    width.resize ((size_t) numChannels);
    for (auto& val : width)
        val.reset (numSteps);

    sat.resize ((size_t) numChannels);
    for (auto& val : sat)
        val.reset (numSteps);

    hProcs.resize ((size_t) numChannels);
    for (size_t ch = 0; ch < (size_t) numChannels; ++ch)
    {
        hProcs[ch].setSampleRate (sampleRate * (double) oversampling->getOversamplingFactor());
        hProcs[ch].cook (drive[ch].getCurrentValue(), width[ch].getCurrentValue(), sat[ch].getCurrentValue());
        hProcs[ch].reset();
    }

    makeup.reset (numSteps);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate * (double) oversampling->getOversamplingFactor();
    spec.maximumBlockSize = (uint32_t) samplesPerBlock;
    spec.numChannels = (uint32_t) numChannels;

    biasParam.prepare (spec);
    driveParam.prepare (spec);
    widthParam.prepare (spec);

    iirFilter.reset();
    *iirFilter.state = juce::dsp::IIR::ArrayCoefficients<double>::makeFirstOrderHighPass(spec.sampleRate, 4.0f);
    iirFilter.prepare(spec);

    // dcBlocker.resize ((size_t) numChannels);
    // for (auto& filt : dcBlocker)
    //     filt.prepare (sampleRate, dcFreq);

    doubleBuffer.setSize (numChannels, samplesPerBlock);

#if HYSTERESIS_USE_SIMD
    const auto maxOSBlockSize = (uint32_t) samplesPerBlock * 16;
    const auto numVecChannels = ceiling_divide ((size_t) numChannels, Vec2::size);
    interleavedBlock = chowdsp::AudioBlock<Vec2> (interleavedBlockData, numVecChannels, maxOSBlockSize);
    zeroBlock = juce::dsp::AudioBlock<double> (zeroData, Vec2::size, maxOSBlockSize);
    zeroBlock.clear();

    channelPointers.resize (numVecChannels * Vec2::size);
#endif
}

float TapeDistortionProcessor::getLatencySamples() const noexcept
{
    // latency of oversampling + fudge factor for hysteresis @TODO: double check this!
    return (float) oversampling->getLatencyInSamples() + 1.4f;
}

void TapeDistortionProcessor::processBlock (juce::dsp::AudioBlock<float>& buffer)
{
    setParameter(TapeDistortionProcessor::Param::Drive, driveParam.getRaw() * 10.0f + 1.0f);
    setParameter(TapeDistortionProcessor::Param::Saturation, driveParam.getRaw());
    setParameter(TapeDistortionProcessor::Param::Bias, 1.0f - powf(widthParam.getRaw(), 2.0f));

    const auto numChannels = buffer.getNumChannels();

    makeup.setTargetValue (calcMakeup());
    bool needsSmoothing = drive[0].isSmoothing() || width[0].isSmoothing() || sat[0].isSmoothing();

    // clip input to avoid unstable hysteresis
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* bufferPtr = buffer.getChannelPointer (ch);
        juce::FloatVectorOperations::clip (bufferPtr,
                                     bufferPtr,
                                     -clipLevel,
                                     clipLevel,
                                     buffer.getNumSamples());
    }

    // Manually copy float buffer to double buffer
    doubleBuffer.setSize(buffer.getNumChannels(), buffer.getNumSamples(), false, false, true);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* src = buffer.getChannelPointer(ch);
        auto* dst = doubleBuffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            dst[i] = static_cast<double>(src[i] + powf(biasParam.getRaw(), 2.0f) * 0.7);
    }

    juce::dsp::AudioBlock<double> block (doubleBuffer);
    juce::dsp::AudioBlock<double> osBlock = oversampling->processSamplesUp (block);

#if HYSTERESIS_USE_SIMD
    const auto n = osBlock.getNumSamples();
    auto* inout = channelPointers.data();
    const auto numChannelsPadded = channelPointers.size();
    for (size_t ch = 0; ch < numChannelsPadded; ++ch)
        inout[ch] = (ch < osBlock.getNumChannels() ? const_cast<double*> (osBlock.getChannelPointer (ch)) : zeroBlock.getChannelPointer (ch % Vec2::size));

    // interleave channels
    for (size_t ch = 0; ch < numChannelsPadded; ch += Vec2::size)
    {
        auto* simdBlockData = reinterpret_cast<double*> (interleavedBlock.getChannelPointer (ch / Vec2::size));
        interleaveSamples (&inout[ch], simdBlockData, static_cast<int> (n), static_cast<int> (Vec2::size));
    }

    auto&& processBlock = interleavedBlock.getSubBlock (0, n);

    using ProcessType = Vec2;
#else
    auto&& processBlock = osBlock;

    using ProcessType = double;
#endif

    if (needsSmoothing)
        processSmooth<ProcessType> (processBlock);
    else
        process<ProcessType> (processBlock);

#if HYSTERESIS_USE_SIMD
    // de-interleave channels
    for (size_t ch = 0; ch < numChannelsPadded; ch += Vec2::size)
    {
        auto* simdBlockData = reinterpret_cast<double*> (interleavedBlock.getChannelPointer (ch / Vec2::size));
        deinterleaveSamples (simdBlockData,
                             const_cast<double**> (&inout[ch]),
                             static_cast<int> (n),
                             static_cast<int> (Vec2::size));
    }
#endif

    oversampling->processSamplesDown (block);

    auto context = juce::dsp::ProcessContextReplacing(block);
    
    // apply DC blocker
    iirFilter.process(context);

    // buffer.makeCopyOf (doubleBuffer, true);
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* src = doubleBuffer.getReadPointer(ch);
        auto* dst = buffer.getChannelPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            dst[i] = static_cast<float>(src[i]);
    }
}

template <typename T, typename SmoothType>
void applyMakeup (juce::dsp::AudioBlock<T>& block, SmoothType& makeup)
{
#if HYSTERESIS_USE_SIMD
    const auto numSamples = block.getNumSamples();
    const auto numChannels = block.getNumChannels();

    if (makeup.isSmoothing())
    {
        for (size_t i = 0; i < numSamples; ++i)
        {
            const auto scaler = makeup.getNextValue();

            for (size_t ch = 0; ch < numChannels; ++ch)
                block.getChannelPointer (ch)[i] *= scaler;
        }
    }
    else
    {
        const auto scaler = makeup.getTargetValue();
        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            auto* x = block.getChannelPointer (ch);
            for (size_t i = 0; i < numSamples; ++i)
                x[i] *= scaler;
        }
    }
#else
    block *= makeup;
#endif
}

template <typename T>
void TapeDistortionProcessor::process (juce::dsp::AudioBlock<T>& block)
{
    const auto numChannels = block.getNumChannels();
    const auto numSamples = block.getNumSamples();

    for (size_t channel = 0; channel < numChannels; ++channel)
    {
        auto* x = block.getChannelPointer (channel);
        auto& hProc = hProcs[channel];
        for (size_t samp = 0; samp < numSamples; samp++) {
            x[samp] = hProc.process (x[samp]);
        }
    }

    applyMakeup<T> (block, makeup);
}

template <typename T>
void TapeDistortionProcessor::processSmooth (juce::dsp::AudioBlock<T>& block)
{
    const auto numChannels = block.getNumChannels();
    const auto numSamples = block.getNumSamples();

    for (size_t channel = 0; channel < numChannels; ++channel)
    {
        auto* x = block.getChannelPointer (channel);
        auto& hProc = hProcs[channel];
        for (size_t samp = 0; samp < numSamples; samp++)
        {
            hProc.cook (drive[channel].getNextValue(), width[channel].getNextValue(), sat[channel].getNextValue());
            x[samp] = hProc.process (x[samp]);
        }
    }

    applyMakeup<T> (block, makeup);
}
