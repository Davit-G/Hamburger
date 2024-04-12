#include "Erosion.h"

 

void Erosion::processBlock(juce::dsp::AudioBlock<float>& block)
{
TRACE_EVENT("dsp", "Erosion::processBlock");

    erosionAmount.update();
    erosionFrequency.update();
    erosionQ.update();

    for (int i = 0; i < block.getNumSamples(); i++) {
        delayLine.pushSample(0, block.getSample(0, i));
        delayLine.pushSample(1, block.getSample(1, i));
    }

    float erosionFreqVal = erosionFrequency.getRaw();
    // get the current value of the saturation knob to figure out how much to multiply a random value by

    float erosionAmtValue = erosionAmount.getRaw() * 0.01f;
    float saturationAmount = erosionAmtValue * erosionAmtValue * 100.0f;

    float q = erosionQ.getRaw();

    // todo: replace all coefficient creation with ArrayCoefficients to avoid allocation
    *iirFilter.coefficients = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass(oldSampleRate, erosionFreqVal, q);

    // now with the filtered values, multiply them by the saturation amount
    float ampVal = 1.0f + powf(erosionFreqVal * 0.0000454545454545f, 2) + (q * 6);

    for (int i = 0; i < block.getNumSamples(); i++) {

        float randomValue = random.nextFloat() * 2.0f - 1.0f;

        float filtered = iirFilter.processSample(randomValue) * ampVal;

        float leftEroded = delayLine.popSample(0, (filtered + 1.f) * saturationAmount);
        float rightEroded = delayLine.popSample(1, (filtered + 1.f) * saturationAmount);

        block.setSample(0, i, leftEroded);
        block.setSample(1, i, rightEroded);
    }
}

void Erosion::prepare(juce::dsp::ProcessSpec& spec)
{
    // init delay
    erosionAmount.prepare(spec);
    erosionFrequency.prepare(spec);
    erosionQ.prepare(spec);

    delayLine.prepare(spec);

    delayLine.setMaximumDelayInSamples(0.1f * spec.sampleRate);

    // init iir filter
    *iirFilter.coefficients = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass(spec.sampleRate, 880.0, 3.0);

    iirFilter.prepare(spec);

    // init random
    random.setSeedRandomly();

    oldSampleRate = spec.sampleRate;
    oldSamplesPerBlock = spec.maximumBlockSize;
}