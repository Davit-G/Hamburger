#include "PostClip.h"
#include "../EffectInfos.h"
#include "../WaveShapers.h"

#include "../../utils/Params.h"


PostClip::PostClip(juce::AudioProcessorValueTreeState &treeState, ScopeDataCollector<float> &scopeDataCollector)
    : MacroEffect(treeState, SlotId{ModuleId::postClip, 0}),
        gainKnob(getParam(ParamIDs::postClipGain)),
        kneeKnob(getParam(ParamIDs::postClipKnee)),
        timeKnob(getParam(ParamIDs::postClipTime)),
        scopeData(scopeDataCollector)
{

    clipEnabled = dynamic_cast<juce::AudioParameterBool *>(treeState.getParameter(slot.enabled().getParamID()));
    type = dynamic_cast<juce::AudioParameterChoice *>(treeState.getParameter(slot.type().getParamID()));
    jassert(clipEnabled && type);
}

PostClip::~PostClip()
{
}

void PostClip::prepare(juce::dsp::ProcessSpec &spec)
{
    gainKnob.prepare(spec);
    kneeKnob.prepare(spec);
    timeKnob.prepare(spec);

    lookahead.prepare(spec.sampleRate);
    wasLimiting = false;
}

int PostClip::getLatencySamples() const
{
    const auto enabled = clipEnabled == nullptr || clipEnabled->get();
    return enabled && type != nullptr && type->getIndex() == limiter ? lookahead.length : 0;
}

/* DONT USE TOGETHER WITH processBlock or smooth value calculations mess up */
void PostClip::processBlock(juce::dsp::AudioBlock<float> &block)
{
    // TRACE_EVENT("dsp", "PostClip::processBlock");
    if (clipEnabled != nullptr && !clipEnabled->get())
    {
        wasLimiting = false;
        return;
    }

    gainKnob.update();
    kneeKnob.update();

    const auto limiting = type != nullptr && type->getIndex() == limiter;

    // whatever it held from the last time it limited is long out of date
    if (limiting && ! wasLimiting)
        lookahead.reset();

    wasLimiting = limiting;
    lookahead.setRelease(timeKnob.getRaw(0));

    for (int sample = 0; sample < block.getNumSamples(); sample++)
    {
        float gainAmount = juce::Decibels::decibelsToGain(gainKnob.getNextValue(0));
        float kneeDb = kneeKnob.getNextValue(0);

        float l = block.getSample(0, sample) * gainAmount;
        float r = block.getSample(1, sample) * gainAmount;

        scopeData.levelMeter.accumulate(fmax(fabs(l), fabs(r)));
        scopeData.clipIndicator.accumulate(fmax(fabs(l), fabs(r)));

        if (limiting)
        {
            lookahead.process(l, r);
        }
        else
        {
            l = softClipperFunc(l, kneeDb);
            r = softClipperFunc(r, kneeDb);
        }

        block.setSample(0, sample, l);
        block.setSample(1, sample, r);
    }
}