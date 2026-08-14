#include "PostClip.h"
#include "../WaveShapers.h"


PostClip::PostClip(juce::AudioProcessorValueTreeState &treeState, ScopeDataCollector<float>& scopeDataCollector) : gainKnob(treeState, ParamIDs::postClipGain),
                                                                    kneeKnob(treeState, ParamIDs::postClipKnee),
                                                                    scopeData(scopeDataCollector)
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
    // TRACE_EVENT("dsp", "PostClip::processBlock");
    if (clipEnabled != nullptr && !clipEnabled->get())
        return;

    gainKnob.update();
    kneeKnob.update();

    for (int sample = 0; sample < block.getNumSamples(); sample++)
    {
        float gainAmount = juce::Decibels::decibelsToGain(gainKnob.getNextValue(0));
        float kneeAmt = kneeKnob.getNextValue(0) * 0.5f;
        
        float l = block.getSample(0, sample) * gainAmount;
        block.setSample(0, sample, softClipperFunc(l, 1.0f, kneeAmt));
        
        float r = block.getSample(1, sample) * gainAmount;
        block.setSample(1, sample, softClipperFunc(r, 1.0f, kneeAmt));
        
        scopeData.accumulateClipping(l, r);
    }
}