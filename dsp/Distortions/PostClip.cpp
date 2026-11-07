#include "PostClip.h"
#include "../EffectInfos.h"
#include "../WaveShapers.h"

#include "../../utils/Params.h"


PostClip::PostClip(juce::AudioProcessorValueTreeState &treeState, ScopeDataCollector<float> &scopeDataCollector)
    : MacroEffect(treeState, SlotId{ModuleId::postClip, 0}),
        gainKnob(getParam(ParamIDs::postClipGain)),
        kneeKnob(getParam(ParamIDs::postClipKnee)),
        timeKnob(getParam(ParamIDs::postClipTime)),
        foldFreqKnob(getParam(ParamIDs::postClipFoldFreq)),
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
    foldFreqKnob.prepare(spec);

    lookahead.prepare(spec.sampleRate);
    wasLimiting = false;

    // 12dB/oct
    foldHighpass.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    foldHighpass.setResonance(1.0f / juce::MathConstants<float>::sqrt2);
    foldHighpass.prepare({ spec.sampleRate, spec.maximumBlockSize, 2 });
    wasFolding = false;
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
        wasFolding = false;
        return;
    }

    gainKnob.update();
    kneeKnob.update();

    const auto current = type != nullptr ? type->getIndex() : soft;
    const auto limiting = current == limiter;
    const auto folding = current == fold || current == foldHighpassed;

    // the knee rounds the first fold's top off under the ceiling, so it's brought back up to it
    if (const auto knee = kneeKnob.getRaw(0); knee != foldMakeupKnee)
    {
        foldMakeupKnee = knee;
        foldMakeup = 1.0f / foldPeak(knee);
    }

    // whatever it held from the last time it limited or folded is long out of date
    if (limiting && ! wasLimiting)
        lookahead.reset();

    if (current == foldHighpassed && ! wasFolding)
        foldHighpass.reset();

    wasLimiting = limiting;
    wasFolding = current == foldHighpassed;
    lookahead.setRelease(timeKnob.getRaw(0));
    foldHighpass.setCutoffFrequency(foldFreqKnob.getRaw(0));

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
        else if (folding)
        {
            /*  Detail preserving clipping: what the clip takes off each channel, the delta, is subtracted from the clipped
                signal straight away, a sidechain ring mod with no attack or release. All of it folds the peaks back under
                the ceiling, through nothing and on down to the other ceiling, see foldBack.

                The highpassed fold runs a highpass on the audio alongside the clip, and folds that top end back instead, only
                where the audio's over the threshold and never more than went over it, so the lows stay clipped flat and
                nothing under the threshold is touched */
            for (auto [channel, x] : { std::pair<int, float*> { 0, &l }, std::pair<int, float*> { 1, &r } })
            {
                if (current == fold)
                {
                    *x = foldShape(*x, kneeDb, foldMakeup);
                    continue;
                }

                const auto clipped = softClipperFunc(*x, kneeDb);
                const auto delta = *x - clipped;

                *x = foldBack(clipped, juce::jlimit(juce::jmin(0.0f, delta), juce::jmax(0.0f, delta), foldHighpass.processSample(channel, *x)), kneeDb);
            }
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