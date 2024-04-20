#pragma once

#include "../EnvelopeFollower.h"

 

class StereoComp
{
public:
    StereoComp(juce::AudioProcessorValueTreeState &state) : 
    // compressorL(CompressionType::COMPRESSOR),
    //                                                     compressorR(CompressionType::COMPRESSOR),
                                                        compressorBoth(CompressionType::COMPRESSOR),
                                                        threshold(state, ParamIDs::stereoCompThreshold),
                                                        ratio(state, ParamIDs::compRatio),
                                                        sLink(state, ParamIDs::compStereoLink), // should be stereo link
                                                        speed(state, ParamIDs::compSpeed),
                                                        makeup(state, ParamIDs::compOut) {}
    ~StereoComp() {}

    void processBlock(dsp::AudioBlock<float> &block)
    {
        TRACE_DSP();

        speed.update();
        makeup.update();
        // sLink.update();
        ratio.update();
        threshold.update();

        float spd = speed.getRaw();
        float mkp = makeup.getRaw();
        float rat = ratio.getRaw();
        // float stereoLink = sLink.getRaw() * 0.01f; // used to be a percentage
        float thr = threshold.getRaw();

        // float atk, float rel, float mkp, float ratioLow, float ratioUp, float thresholdLow, float thresholdUp, float kneeW, float mkpDB)
        // compressorL.updateUpDown(spd, spd * 0.8f, mkp, rat, rat, thr, thr + 2.0f, 0.1f, 0.f);
        // compressorR.updateUpDown(spd, spd * 0.8f, mkp, rat, rat, thr, thr + 2.0f, 0.1f, 0.f);
        compressorBoth.updateUpDown(spd, spd * 0.8f, mkp, rat, rat, thr, thr + 2.0f, 0.1f, 0.f);

        float autoGain = juce::Decibels::decibelsToGain(-thr * powf((rat - 1.0f) * 0.09f, 0.4f) * 0.45); // kinda borked

        for (int sample = 0; sample < block.getNumSamples(); sample++)
        {
            float leftSample = block.getSample(0, sample);
            float rightSample = block.getSample(1, sample);

            float bothGain = compressorBoth.processOneSampleGainStereo(leftSample, rightSample);

            // left right stereo link. tlt is between 0 and 1
            float stereoLinkGain = bothGain * autoGain;

            block.setSample(0, sample, stereoLinkGain * leftSample);
            block.setSample(1, sample, stereoLinkGain * rightSample);
        }
    }

    void prepare(dsp::ProcessSpec &spec)
    {
        // compressorL.prepare(spec);
        // compressorR.prepare(spec);
        compressorBoth.prepare(spec);
    }

private:
    double sampleRate;

    SmoothParam threshold;
    SmoothParam ratio;
    SmoothParam sLink;
    SmoothParam speed;
    SmoothParam makeup;

    // Compressor compressorL;
    // Compressor compressorR;
    Compressor compressorBoth;
};