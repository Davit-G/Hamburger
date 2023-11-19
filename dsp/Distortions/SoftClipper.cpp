#include "SoftClipper.h"

template <typename T>
int sgn(T val)
{
    return (T(0) < val) - (val < T(0));
}

SoftClip::SoftClip(juce::AudioProcessorValueTreeState& treeState) : saturationKnob(treeState, "saturationAmount") {}

SoftClip::~SoftClip()
{
}

void SoftClip::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    saturationKnob.prepareToPlay(sampleRate, samplesPerBlock);
}

/* DONT USE TOGETHER WITH processBlock or smooth value calculations mess up */
void SoftClip::legacyProcessBlock(dsp::AudioBlock<float>& block) {
    saturationKnob.update();

    for (int sample = 0; sample < block.getNumSamples(); sample++)
    {
        float saturationAmount = saturationKnob.getNextValue();

        float xn = block.getSample(0, sample);
        block.setSample(0, sample, sgn(xn) * (1.0 - exp(-fabs((saturationAmount * 0.1f + 1.f) * xn)))); // multiply audio on both channels by gain

        xn = block.getSample(1, sample);
        block.setSample(1, sample, sgn(xn) * (1.0 - exp(-fabs((saturationAmount * 0.1f + 1.f) * xn)))); // multiply audio on both channels by gain
    }
}

void SoftClip::processBlock(dsp::AudioBlock<float>& block) {
    TRACE_EVENT("dsp", "SoftClip::processBlock");
    saturationKnob.update();

    for (int sample = 0; sample < block.getNumSamples(); sample++)
    {
        float saturationAmount = saturationKnob.getNextValue() * 0.01;

        float xn = block.getSample(0, sample);
        block.setSample(0, sample, softClipper(xn * (1.f + saturationAmount * 5.0f), 0.9f, 0.2f, (saturationAmount + 0.05) * 20.0f));

        xn = block.getSample(1, sample);
        block.setSample(1, sample, softClipper(xn * (1.f + saturationAmount * 5.0f), 0.9f, 0.2f, (saturationAmount + 0.05) * 20.0f));
    }
}

// the proper one, with knee and all that
float SoftClip::softClipper(float x, float threshold, float knee, float ratio = 100.f)
{
    // this is the soft-clip function from the compressor
    // https://www.desmos.com/calculator/f8zazgtwpe

    float sign = sgn(x); // rectify it so we stay in the positive domain
    x = fabs(x);

    float output = 0.0;

    if (2 * (x - threshold) < -knee)
    {
        // below knee
        output = x;
    }
    else if (2 * (x - threshold) > knee)
    {
        output = threshold + (x - threshold) / ratio;
    }
    else
    {
        output = x + ((1 / ratio - 1) * pow((x - threshold + knee * 0.5), 2) / (2 * knee));
    }

    return output * sign; // return it to the original sign afterwards
}