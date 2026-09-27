#pragma once

#include "../EnvelopeFollower.h"
#include "../../gui/Modules/ScopeDataCollector.h"
#include "../EffectBase.h"
#include "../EffectInfos.h"

 

class StereoComp : public MacroEffect
{
public:
    StereoComp(juce::AudioProcessorValueTreeState &state, ScopeDataCollector<float> &dataCollector)
                                                      : MacroEffect(state, SlotId{ModuleId::dynamics, 0}),
                                                        compressorL(CompressionType::COMPRESSOR),
                                                        compressorR(CompressionType::COMPRESSOR),
                                                        threshold(getParam(ParamIDs::stereoCompThreshold)),
                                                        ratio(getParam(ParamIDs::compRatio)),
                                                        link(getParam(ParamIDs::compStereoLink)),
                                                        speed(getParam(ParamIDs::compSpeed)),
                                                        scopeDataCollector(dataCollector) {}
    ~StereoComp() {}

    void processBlock(juce::dsp::AudioBlock<float> &block) override
    {
        float spd = speed.getRaw(0);
        float rat = ratio.getRaw(0);
        float thr = threshold.getRaw(0);
        float linked = link.getRaw(0) * 0.01f;

        // float atk, float rel, float mkp, float ratioLow, float ratioUp, float thresholdLow, float thresholdUp, float kneeW, float mkpDB)
        for (auto *comp : { &compressorL, &compressorR })
            comp->updateUpDown(spd, spd * 0.8f, 0.0f, rat, rat, thr, thr + 2.0f, Compressor::standardKneeDb, 0.f);

        float autoGain = juce::Decibels::decibelsToGain(-thr * powf((rat - 1.0f) * 0.09f, 0.4f) * 0.45); // kinda borked

        for (int sample = 0; sample < block.getNumSamples(); sample++)
        {
            float leftSample = block.getSample(0, sample);
            float rightSample = block.getSample(1, sample);

            // each side hears itself, or both together at full link, so at full link they get the same gain
            float both = (std::abs(leftSample) + std::abs(rightSample)) * 0.5f;
            float heardL = std::abs(leftSample) + (both - std::abs(leftSample)) * linked;
            float heardR = std::abs(rightSample) + (both - std::abs(rightSample)) * linked;

            float gainL = compressorL.processOneSampleGainStereo(heardL, heardL);
            float gainR = compressorR.processOneSampleGainStereo(heardR, heardR);

            scopeDataCollector.band1.accumulateDb(compressorL.lastEnvelopeDb);
            scopeDataCollector.band2.accumulateDb(compressorR.lastEnvelopeDb);

            block.setSample(0, sample, leftSample * gainL * autoGain);
            block.setSample(1, sample, rightSample * gainR * autoGain);
        }
    }

    void prepare(juce::dsp::ProcessSpec &spec)
    {
        compressorL.prepare(spec);
        compressorR.prepare(spec);
    }

private:
    Compressor compressorL;
    Compressor compressorR;

    SmoothParam threshold;
    SmoothParam ratio;
    SmoothParam link;
    SmoothParam speed;

    ScopeDataCollector<float> &scopeDataCollector;
};
