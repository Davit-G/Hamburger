#pragma once

#include "../EnvelopeFollower.h"
#include "../../gui/Modules/ScopeDataCollector.h"
#include "../EffectBase.h"
#include "../EffectInfos.h"
#include "ThreeBands.h"
 

class MBComp : public MacroEffect
{
public:
    MBComp(juce::AudioProcessorValueTreeState &state, ScopeDataCollector<float> &dataCollector, ThreeBands &threeBands)
                                                      : MacroEffect(state, SlotId{ModuleId::dynamics, 0}),
                                                        compressor1(CompressionType::COMPRESSOR),
                                                        compressor2(CompressionType::COMPRESSOR),
                                                        compressor3(CompressionType::COMPRESSOR),
                                                        threshold(getParam(ParamIDs::MBCompThreshold)),
                                                        ratio(getParam(ParamIDs::compRatio)),
                                                        tilt(getParam(ParamIDs::compBandTilt)),
                                                        speed(getParam(ParamIDs::MBCompSpeed)),
                                                        scopeDataCollector(dataCollector),
                                                        bands(threeBands) {}
    ~MBComp() {}

    void processBlock(juce::dsp::AudioBlock<float> &block) override
    {
        float spd = speed.getRaw(0);
        float rat = ratio.getRaw(0);
        float tlt = tilt.getRaw(0);
        float thr = threshold.getRaw(0);

        // float atk, float rel, float mkp, float ratioLow, float ratioUp, float thresholdLow, float thresholdUp, float kneeW, float mkpDB)
        compressor1.updateUpDown(spd, spd * 0.8f, 0.0f, rat, rat, thr - tlt, thr + 2.0f - tlt, Compressor::standardKneeDb, 0.f);
        compressor2.updateUpDown(spd, spd * 0.8f, 0.0f, rat, rat, thr, thr + 2.0f, Compressor::standardKneeDb, 0.f);
        compressor3.updateUpDown(spd, spd * 0.8f, 0.0f, rat, rat, thr + tlt, thr + 2.0f - tlt, Compressor::standardKneeDb, 0.f);

        float autoGain = juce::Decibels::decibelsToGain(-thr * powf((rat - 1.0f) * 0.09f, 0.4f) * 0.45);

        bands.split(block);

        auto &low = bands.bands[0], &mid = bands.bands[1], &high = bands.bands[2];

        for (int sample = 0; sample < (int) block.getNumSamples(); sample++)
        {
            float lowGain = compressor1.processOneSampleGainStereo(low.getSample(0, sample), low.getSample(1, sample));
            float midGain = compressor2.processOneSampleGainStereo(mid.getSample(0, sample), mid.getSample(1, sample));
            float highGain = compressor3.processOneSampleGainStereo(high.getSample(0, sample), high.getSample(1, sample));

            scopeDataCollector.band1.accumulateDb(compressor1.lastEnvelopeDb);
            scopeDataCollector.band2.accumulateDb(compressor2.lastEnvelopeDb);
            scopeDataCollector.band3.accumulateDb(compressor3.lastEnvelopeDb);

            for (int ch = 0; ch < 2; ++ch)
                block.setSample(ch, sample, (low.getSample(ch, sample) * lowGain + mid.getSample(ch, sample) * midGain + high.getSample(ch, sample) * highGain) * autoGain);
        }
    }

    void prepare(juce::dsp::ProcessSpec &spec) override
    {
        compressor1.prepare(spec);
        compressor2.prepare(spec);
        compressor3.prepare(spec);
    }

private:
    Compressor compressor1;
    Compressor compressor2;
    Compressor compressor3;

    SmoothParam threshold;
    SmoothParam ratio;
    SmoothParam tilt;
    SmoothParam speed;

    ScopeDataCollector<float> &scopeDataCollector;
    ThreeBands &bands;
};
