#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
 */
class PreReverb
{
public:
    PreReverb(juce::AudioParameterFloat* param, juce::AudioParameterFloat* param2, juce::AudioParameterFloat* param3, juce::AudioParameterFloat* param4) {
        reverbMix = param;
        jassert(reverbMix);

        reverbSize = param2;
        jassert(reverbSize);

        reverbWidth = param3;
        jassert(reverbWidth);

        reverbDamping = param4;
        jassert(reverbDamping);
    }
    ~PreReverb() {

    };

    void processBlock(dsp::AudioBlock<float>& block) {
        if ((oldReverbMix != reverbMix->get()) || (oldReverbDamping != reverbDamping->get()) || (oldReverbSize != reverbSize->get()) || (oldReverbWidth != reverbWidth->get())) {
            juce::Reverb::Parameters parameters;
            parameters.width = reverbWidth->get();
            parameters.freezeMode = 0.0;
            parameters.damping = reverbDamping->get();
            parameters.roomSize = reverbSize->get();
            parameters.wetLevel = 1.0;
            parameters.dryLevel = 0.0;

            reverb.setParameters(parameters);

            
            oldReverbSize = parameters.roomSize;
            oldReverbWidth = parameters.width;
            oldReverbDamping = parameters.damping;
        }

        reverb.processStereo(block.getChannelPointer(0), block.getChannelPointer(1), block.getNumSamples());
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) {
        juce::Reverb::Parameters parameters;
        parameters.width = 1.0;
        parameters.freezeMode = 0.0;
        parameters.damping = 0.3f;
        parameters.roomSize = 1.0;
        parameters.wetLevel = 1.0;
        parameters.dryLevel = 0.0;

        reverb.setParameters(parameters);
        reverb.reset();
        reverb.setSampleRate(sampleRate);
    }

private:
    juce::AudioParameterFloat *reverbMix = nullptr;
    juce::AudioParameterFloat *reverbSize = nullptr;
    juce::AudioParameterFloat *reverbWidth = nullptr;
    juce::AudioParameterFloat *reverbDamping = nullptr;

    float oldReverbMix = 0.0;
    float oldReverbSize = 0.0;
    float oldReverbWidth = 0.0;
    float oldReverbDamping = 0.0;

    juce::Reverb reverb;

    double oldSampleRate;
    double oldSamplesPerBlock;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PreReverb)
};