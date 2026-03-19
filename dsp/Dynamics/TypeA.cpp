#include "TypeA.h"

void TypeAProcessor::prepareToPlay(double sampleRate, int samplesPerBlock, int numChannels)
{
    this->currentSampleRate = sampleRate;
    this->numChannels = numChannels;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(numChannels);

    tiltParam.prepare(spec);
    brightnessParam.prepare(spec);
    thresholdParam.prepare(spec);
    outParam.prepare(spec);

    // Initialize crossover filters with 12 dB/octave (2nd order)
    *lowPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, lowCrossover);
    *highPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, midHighCrossover);
    *extraHighPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, extraHighCrossover);

    lowPassFilter.prepare(spec);
    highPassFilter.prepare(spec);
    extraHighPassFilter.prepare(spec);

    // Initialize compressors
    lowCompressor.prepare(spec);
    midCompressor.prepare(spec);
    highCompressor.prepare(spec);
    extraHighCompressor.prepare(spec);

    // Initialize makeup gains
    lowMakeupGain.prepare(spec);
    midMakeupGain.prepare(spec);
    highMakeupGain.prepare(spec);
    extraHighMakeupGain.prepare(spec);

    // Allocate temp buffers
    lowBuffer.setSize(numChannels, samplesPerBlock);
    midBuffer.setSize(numChannels, samplesPerBlock);
    highBuffer.setSize(numChannels, samplesPerBlock);
    extraHighBuffer.setSize(numChannels, samplesPerBlock);

    updateCompressorParameters();
}

void TypeAProcessor::processBlock(juce::dsp::AudioBlock<float>& buffer)
{
    updateCompressorParameters();

    const auto numSamples = static_cast<int>(buffer.getNumSamples());
    const auto numChans = static_cast<int>(buffer.getNumChannels());

    // Copy input to band buffers for filtering
    for (int ch = 0; ch < numChans; ++ch)
    {
        lowBuffer.copyFrom(ch, 0, buffer.getChannelPointer(static_cast<size_t>(ch)), numSamples);
        highBuffer.copyFrom(ch, 0, buffer.getChannelPointer(static_cast<size_t>(ch)), numSamples);
        extraHighBuffer.copyFrom(ch, 0, buffer.getChannelPointer(static_cast<size_t>(ch)), numSamples);
    }

    // Create audio blocks for each band
    juce::dsp::AudioBlock<float> lowBlock(lowBuffer);
    juce::dsp::AudioBlock<float> highBlock(highBuffer);
    juce::dsp::AudioBlock<float> extraHighBlock(extraHighBuffer);

    // Apply crossover filters
    // Low band: LP at 80 Hz
    lowPassFilter.process(juce::dsp::ProcessContextReplacing<float>(lowBlock));

    // High band: HP at 3 kHz
    highPassFilter.process(juce::dsp::ProcessContextReplacing<float>(highBlock));

    // Extra high band: HP at 9 kHz
    extraHighPassFilter.process(juce::dsp::ProcessContextReplacing<float>(extraHighBlock));

    // Mid band: signal - low - high (derived, no extra filters needed)
    for (int ch = 0; ch < numChans; ++ch)
    {
        auto* mid = midBuffer.getWritePointer(ch);
        const auto* input = buffer.getChannelPointer(static_cast<size_t>(ch));
        const auto* low = lowBuffer.getReadPointer(ch);
        const auto* high = highBuffer.getReadPointer(ch);

        juce::FloatVectorOperations::copy(mid, input, numSamples);
        juce::FloatVectorOperations::add(mid, low, numSamples);
        juce::FloatVectorOperations::subtract(mid, high, numSamples);
    }

    juce::dsp::AudioBlock<float> midBlock(midBuffer);

    // Apply compression to each band
    lowCompressor.process(juce::dsp::ProcessContextReplacing<float>(lowBlock));
    midCompressor.process(juce::dsp::ProcessContextReplacing<float>(midBlock));
    highCompressor.process(juce::dsp::ProcessContextReplacing<float>(highBlock));
    extraHighCompressor.process(juce::dsp::ProcessContextReplacing<float>(extraHighBlock));

    // Apply makeup gain
    lowMakeupGain.process(juce::dsp::ProcessContextReplacing<float>(lowBlock));
    midMakeupGain.process(juce::dsp::ProcessContextReplacing<float>(midBlock));
    highMakeupGain.process(juce::dsp::ProcessContextReplacing<float>(highBlock));
    extraHighMakeupGain.process(juce::dsp::ProcessContextReplacing<float>(extraHighBlock));

    // Sum all bands back together
    for (int ch = 0; ch < numChans; ++ch)
    {
        auto* output = buffer.getChannelPointer(static_cast<size_t>(ch));
        const auto* low = lowBuffer.getReadPointer(ch);
        const auto* mid = midBuffer.getReadPointer(ch);
        const auto* high = highBuffer.getReadPointer(ch);
        const auto* extraHigh = extraHighBuffer.getReadPointer(ch);

        // Start with low band
        juce::FloatVectorOperations::copyWithMultiply(output, low, -1.0f, numSamples);
        // Add mid band
        juce::FloatVectorOperations::add(output, mid, numSamples);
        // Add high band
        juce::FloatVectorOperations::subtract(output, high, numSamples);
        // Add extra high band (stacked on high for additional noise reduction above 9 kHz)
        juce::FloatVectorOperations::subtract(output, extraHigh, numSamples);
    }
}

void TypeAProcessor::updateCompressorParameters()
{
    const float brightness = brightnessParam.getRaw();
    const float threshold = thresholdParam.getRaw();
    // const float speed = speedParam.getRaw();
    const float tilt = tiltParam.getRaw();

    const float outValue = outParam.getRaw();

    // Brightness affects ratio: 0 = baseRatio, 1 = higher ratio (more compression on highs)
    const float ratio = 1.0f + brightness * (baseRatio - 1.0f);

    // Set compressor parameters
    lowCompressor.setThreshold(threshold);
    lowCompressor.setRatio(ratio);
    lowCompressor.setAttack(attack);
    lowCompressor.setRelease(release);

    midCompressor.setThreshold(threshold);
    midCompressor.setRatio(ratio);
    midCompressor.setAttack(attack);
    midCompressor.setRelease(release);

    highCompressor.setThreshold(threshold);
    highCompressor.setRatio(ratio);
    highCompressor.setAttack(attack);
    highCompressor.setRelease(release);

    extraHighCompressor.setThreshold(threshold);
    extraHighCompressor.setRatio(ratio);
    extraHighCompressor.setAttack(attack);
    extraHighCompressor.setRelease(release);

    const float makeupGain = calculateMakeupGain(ratio, threshold) + outValue;

    // Set makeup gains based on compression ratio
    lowMakeupGain.setGainDecibels(makeupGain - tilt);
    midMakeupGain.setGainDecibels(makeupGain);
    highMakeupGain.setGainDecibels(makeupGain + tilt * 0.5f);
    extraHighMakeupGain.setGainDecibels(makeupGain + tilt * 0.5f);
}

float TypeAProcessor::calculateMakeupGain(float ratio, float thres) const
{
    return -thres * (1.0f - 1.0f / ratio);
}
