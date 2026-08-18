#pragma once

#include "../EnvelopeFollower.h"
#include "../../gui/Modules/ScopeDataCollector.h"

 

class StereoComp
{
public:
    StereoComp(juce::AudioProcessorValueTreeState &state, ScopeDataCollector<float> &dataCollector) : 
                                                        detectorL(CompressionType::COMPRESSOR),
                                                        detectorR(CompressionType::COMPRESSOR),
                                                        compressorBoth(CompressionType::COMPRESSOR),
                                                        threshold(state, ParamIDs::stereoCompThreshold),
                                                        ratio(state, ParamIDs::compRatio),
                                                        sLink(state, ParamIDs::compStereoLink), // should be stereo link
                                                        speed(state, ParamIDs::compSpeed),
                                                        makeup(state, ParamIDs::compOut),
                                                        scopeDataCollector(dataCollector) {}
    ~StereoComp() {}

    void processBlock(juce::dsp::AudioBlock<float> &block)
    {
        speed.update();
        makeup.update();
        // sLink.update();
        ratio.update();
        threshold.update();

        float spd = speed.getRaw(0);
        float mkp = makeup.getRaw(0);
        float rat = ratio.getRaw(0);
        // float stereoLink = sLink.getRaw() * 0.01f; // used to be a percentage
        float thr = threshold.getRaw(0);

        // float atk, float rel, float mkp, float ratioLow, float ratioUp, float thresholdLow, float thresholdUp, float kneeW, float mkpDB)
        // the detectors need the same setup as the audio path so their meters agree with it
        for (auto *comp : { &compressorBoth, &detectorL, &detectorR })
            comp->updateUpDown(spd, spd * 0.8f, mkp, rat, rat, thr, thr + 2.0f, Compressor::standardKneeDb, 0.f);

        float autoGain = juce::Decibels::decibelsToGain(-thr * powf((rat - 1.0f) * 0.09f, 0.4f) * 0.45); // kinda borked

        for (int sample = 0; sample < block.getNumSamples(); sample++)
        {
            float leftSample = block.getSample(0, sample);
            float rightSample = block.getSample(1, sample);

            float bothGain = compressorBoth.processOneSampleGainStereo(leftSample, rightSample);

            scopeDataCollector.band1.accumulateDb(detectorL.detectMono(leftSample));
            scopeDataCollector.band2.accumulateDb(detectorR.detectMono(rightSample));

            // left right stereo link. tlt is between 0 and 1
            float stereoLinkGain = bothGain * autoGain;

            block.setSample(0, sample, stereoLinkGain * leftSample);
            block.setSample(1, sample, stereoLinkGain * rightSample);
        }
    }

    void prepare(juce::dsp::ProcessSpec &spec)
    {
        detectorL.prepare(spec);
        detectorR.prepare(spec);
        compressorBoth.prepare(spec);
    }

private:
    double sampleRate;

    SmoothParam threshold;
    SmoothParam ratio;
    SmoothParam sLink;
    SmoothParam speed;
    SmoothParam makeup;

    // metering only, the audio never goes through these
    Compressor detectorL;
    Compressor detectorR;
    Compressor compressorBoth;

    ScopeDataCollector<float> &scopeDataCollector;
};