#pragma once

#include "../EnvelopeFollower.h"

 

class MBComp
{
public:
    MBComp(juce::AudioProcessorValueTreeState &state) : compressor1(CompressionType::COMPRESSOR),
                                                        compressor2(CompressionType::COMPRESSOR),
                                                        compressor3(CompressionType::COMPRESSOR),
                                                        threshold(state, ParamIDs::MBCompThreshold),
                                                        ratio(state, ParamIDs::compRatio),
                                                        tilt(state, ParamIDs::compBandTilt),
                                                        speed(state, ParamIDs::compSpeed),
                                                        makeup(state, ParamIDs::compOut) {}
    ~MBComp() {}

    void processBlock(dsp::AudioBlock<float> &block)
    {
        TRACE_DSP();

        speed.update();
        makeup.update();
        tilt.update();
        ratio.update();
        threshold.update();

        float spd = speed.getRaw();
        float mkp = makeup.getRaw();
        float rat = ratio.getRaw();
        float tlt = tilt.getRaw();
        float thr = threshold.getRaw();

        // float atk, float rel, float mkp, float ratioLow, float ratioUp, float thresholdLow, float thresholdUp, float kneeW, float mkpDB)
        compressor1.updateUpDown(spd, spd * 0.8f, mkp, rat, rat, thr - tlt, thr + 2.0f - tlt, 0.1f, 0.f);
        compressor2.updateUpDown(spd, spd * 0.8f, mkp, rat, rat, thr, thr + 2.0f, 0.1f, 0.f);
        compressor3.updateUpDown(spd, spd * 0.8f, mkp, rat, rat, thr + tlt, thr + 2.0f - tlt, 0.1f, 0.f);

        float autoGain = juce::Decibels::decibelsToGain(-thr * powf((rat - 1.0f) * 0.09f, 0.4f) * 0.45);

        for (int sample = 0; sample < block.getNumSamples(); sample++)
        {
            float leftSample = block.getSample(0, sample);
            float rightSample = block.getSample(1, sample);
            
            lowCrossOver.processSample(0, leftSample, lowResultL, notLowL);
            highCrossOver.processSample(0, notLowL, midResultL, highResultL);

            lowCrossOver.processSample(1, rightSample, lowResultR, notLowR);
            highCrossOver.processSample(1, notLowR, midResultR, highResultR);

            float lowGain = compressor1.processOneSampleGainStereo(lowResultL, lowResultR);
            float midGain = compressor2.processOneSampleGainStereo(midResultL, midResultR);
            float highGain = compressor3.processOneSampleGainStereo(highResultL, highResultR);

            block.setSample(0, sample, (lowResultL * lowGain + midResultL * midGain + highResultL * highGain) * autoGain);
            block.setSample(1, sample, (lowResultR * lowGain + midResultR * midGain + highResultR * highGain) * autoGain);
        }
    }

    void prepare(dsp::ProcessSpec &spec)
    {
        compressor1.prepare(spec);
        compressor2.prepare(spec);
        compressor3.prepare(spec);

        lowCrossOver.setCutoffFrequency(200.0f);
        highCrossOver.setCutoffFrequency(3000.0f);
        lowCrossOver.setType(dsp::LinkwitzRileyFilterType::lowpass);
        highCrossOver.setType(dsp::LinkwitzRileyFilterType::highpass);
        lowCrossOver.prepare(spec);
        highCrossOver.prepare(spec);
    }

private:
    Compressor compressor1;
    Compressor compressor2;
    Compressor compressor3;

    SmoothParam threshold;
    SmoothParam ratio;
    SmoothParam tilt;
    SmoothParam speed;
    SmoothParam makeup;

    float lowResultL = 0.0f;
    float notLowL = 0.0f;
    float midResultL = 0.0f;
    float highResultL = 0.0f;
    float lowResultR = 0.0f;
    float notLowR = 0.0f;
    float midResultR = 0.0f;
    float highResultR = 0.0f;

    dsp::LinkwitzRileyFilter<float> lowCrossOver;
    dsp::LinkwitzRileyFilter<float> highCrossOver;
};