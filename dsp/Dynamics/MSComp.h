#pragma once

#include "../EnvelopeFollower.h"

 

// mid side compression
class MSComp
{
public:
    MSComp(juce::AudioProcessorValueTreeState &state) : compressorMid(CompressionType::COMPRESSOR),
                                                        compressorSide(CompressionType::COMPRESSOR),
                                                        threshold(state, ParamIDs::MSCompThreshold),
                                                        ratio(state, ParamIDs::compRatio),
                                                        tilt(state, ParamIDs::compBandTilt),
                                                        speed(state, ParamIDs::compSpeed),
                                                        makeup(state, ParamIDs::compOut) {}
    ~MSComp() {}

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
        compressorMid.updateUpDown(spd, spd * 0.8f, mkp, rat, rat, thr - tlt, thr + 2.0f - tlt, 0.1f, 0.f);
        compressorSide.updateUpDown(spd, spd * 0.8f, mkp, rat, rat, thr + tlt, thr + 2.0f + tlt, 0.1f, 0.f);

        float autoGain = juce::Decibels::decibelsToGain(-thr * powf((rat - 1.0f) * 0.09f, 0.4f) * 0.45); // kinda borked

        for (int sample = 0; sample < block.getNumSamples(); sample++)
        {
            float leftSample = block.getSample(0, sample);
            float rightSample = block.getSample(1, sample);

            float midSample = (leftSample + rightSample) * 0.5f;
            float sideSample = (leftSample - rightSample) * 0.5f;

            float midGain = compressorMid.processOneSampleGainMono(midSample);
            float sideGain = compressorSide.processOneSampleGainMono(sideSample);

            float midOut = midSample * midGain;
            float sideOut = sideSample * sideGain;

            float leftOut = midOut + sideOut;
            float rightOut = midOut - sideOut;

            block.setSample(0, sample, leftOut * autoGain);
            block.setSample(1, sample, rightOut * autoGain);
        }
    }

    void prepare(dsp::ProcessSpec &spec)
    {
        compressorMid.prepare(spec);
        compressorSide.prepare(spec);
    }

private:
    double sampleRate;

    SmoothParam threshold;
    SmoothParam ratio;
    SmoothParam tilt;
    SmoothParam speed;
    SmoothParam makeup;

    Compressor compressorMid;
    Compressor compressorSide;
};