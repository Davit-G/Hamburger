#pragma once

#include "../EnvelopeFollower.h"

 

class MBComp
{
public:
    MBComp(juce::AudioProcessorValueTreeState &state) : compressor1(CompressionType::COMPRESSOR),
                                                        compressor2(CompressionType::COMPRESSOR),
                                                        compressor3(CompressionType::COMPRESSOR),
                                                        threshold(state, ParamIDs::compThreshold),
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

            float lowResultL = 0.0f;
            float notLowL = 0.0f;
            lowCrossOver.processSample(0, leftSample, lowResultL, notLowL);
            float midResultL = 0.0f;
            float highResultL = 0.0f;
            highCrossOver.processSample(0, notLowL, midResultL, highResultL);

            float lowResultR = 0.0f;
            float notLowR = 0.0f;
            lowCrossOver.processSample(1, rightSample, lowResultR, notLowR);
            float midResultR = 0.0f;
            float highResultR = 0.0f;
            highCrossOver.processSample(1, notLowR, midResultR, highResultR);

            float lowGain = compressor1.processOneSampleGainStereo(lowResultL, lowResultR);
            float midGain = compressor2.processOneSampleGainStereo(midResultL, midResultR);
            float highGain = compressor3.processOneSampleGainStereo(highResultL, highResultR);

            lowBuffer.setSample(0, sample, lowResultL * lowGain);
            midBuffer.setSample(0, sample, midResultL * midGain);
            highBuffer.setSample(0, sample, highResultL * highGain);
            lowBuffer.setSample(1, sample, lowResultR * lowGain);
            midBuffer.setSample(1, sample, midResultR * midGain);
            highBuffer.setSample(1, sample, highResultR * highGain);
        }

        block.copyFrom(lowBlock);
        block.addProductOf(midBlock, 1.0f);
        block.addProductOf(highBlock, 1.0f);
        block.multiplyBy(autoGain);
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

        lowBuffer.setSize(2, spec.maximumBlockSize);
        midBuffer.setSize(2, spec.maximumBlockSize);
        highBuffer.setSize(2, spec.maximumBlockSize);

        lowBlock = dsp::AudioBlock<float>(lowBuffer);
        midBlock = dsp::AudioBlock<float>(midBuffer);
        highBlock = dsp::AudioBlock<float>(highBuffer);
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

    AudioBuffer<float> lowBuffer;
    AudioBuffer<float> midBuffer;
    AudioBuffer<float> highBuffer;
    dsp::AudioBlock<float> lowBlock;
    dsp::AudioBlock<float> midBlock;
    dsp::AudioBlock<float> highBlock;

    // mid-high 2.5khz
    // low - mid 500hz

    dsp::LinkwitzRileyFilter<float> lowCrossOver;
    dsp::LinkwitzRileyFilter<float> highCrossOver;
};