#include "PostClip.h"
#include "WaveShapers.h"

template <typename T>
int sgn(T val)
{
    return (T(0) < val) - (val < T(0));
}

PostClip::PostClip(juce::AudioProcessorValueTreeState &treeState) : gainKnob(treeState, ParamIDs::postClipGain),
                                                                    kneeKnob(treeState, ParamIDs::postClipKnee)
{
    clipEnabled = dynamic_cast<juce::AudioParameterBool *>(treeState.getParameter(ParamIDs::postClipEnabled.getParamID()));
    jassert(clipEnabled);
}

PostClip::~PostClip()
{
}

void PostClip::prepare(juce::dsp::ProcessSpec &spec)
{
    gainKnob.prepare(spec);
    kneeKnob.prepare(spec);
}

/* DONT USE TOGETHER WITH processBlock or smooth value calculations mess up */
void PostClip::processBlock(juce::dsp::AudioBlock<float> &block)
{
    TRACE_EVENT("dsp", "PostClip::processBlock");

    gainKnob.update();
    kneeKnob.update();

    for (int sample = 0; sample < block.getNumSamples(); sample++)
    {
        float gainAmount = juce::Decibels::decibelsToGain(gainKnob.getNextValue());
        float kneeAmt = kneeKnob.getNextValue() * 0.5f;

        float xn = block.getSample(0, sample) * gainAmount;
        block.setSample(0, sample, softClipperFunc(xn, 1.0f, kneeAmt));

        xn = block.getSample(1, sample) * gainAmount;
        block.setSample(1, sample, softClipperFunc(xn, 1.0f, kneeAmt));
    }
}