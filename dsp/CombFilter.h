#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
 */
class CombFilter
{
public:
    CombFilter(juce::AudioParameterFloat* param, juce::AudioParameterFloat* param2, juce::AudioParameterFloat* param3) : delayLine(10000) {
        combDelay = param;
        jassert(combDelay);

        combFeedback = param2;
        jassert(combFeedback);

        combMix = param3;
        jassert(combMix);
    }
    ~CombFilter() {

    };

    void processBlock(dsp::AudioBlock<float>& block) {

        delayLine.setDelay(combDelay->get());

        float feedback = combFeedback->get();
        
        for (int i = 0; i < block.getNumSamples(); i++) {
            float outputL = delayLine.popSample(0);
            float outputR = delayLine.popSample(1);

            delayLine.pushSample(0, block.getSample(0, i) + feedback * outputL);
            delayLine.pushSample(1, block.getSample(1, i) + feedback * outputR );

            block.setSample(0, i, block.getSample(0, i) + outputL);
            block.setSample(1, i, block.getSample(1, i) + outputR);
        }

    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) {
        dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = samplesPerBlock;
        spec.numChannels = 2;

        delayLine.prepare(spec);
    }

private:
    juce::AudioParameterFloat *combDelay = nullptr;
    juce::AudioParameterFloat *combFeedback = nullptr;
    juce::AudioParameterFloat *combMix = nullptr;

    float oldCombDelay = 0.0;
    float oldCombFeedback = 0.0;
    float oldCombMix = 0.0;

    dsp::DelayLine<float, dsp::DelayLineInterpolationTypes::Linear> delayLine;

    double oldSampleRate;
    double oldSamplesPerBlock;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CombFilter)
};