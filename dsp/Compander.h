#pragma once

#include "EnvelopeFollower.h"

#include <JuceHeader.h>

class Compander
{
public:
    Compander(juce::AudioProcessorValueTreeState &state) : compressorEnv(true), expanderEnv(true), treeState(state)
    {
        attackKnob = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter("compAttack"));
        jassert(attackKnob); // heads up if the parameter doesn't exist

        releaseKnob = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter("compRelease"));
        jassert(releaseKnob); // heads up if the parameter doesn't exist

        thresholdKnob = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter("compThreshold"));
        jassert(thresholdKnob); // heads up if the parameter doesn't exist

        ratioKnob = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter("compRatio"));
        jassert(ratioKnob); // heads up if the parameter doesn't exist

        makeupKnob = dynamic_cast<juce::AudioParameterFloat *>(treeState.getParameter("compOut"));
        jassert(makeupKnob); // heads up if the parameter doesn't exist
    }
    
    ~Compander(){}

    void updateParameters();
    void processExpanderBlock(juce::AudioBuffer<float> &dryBuffer);
    void processCompressorBlock(juce::AudioBuffer<float> &dryBuffer);
    void processUpwardsDownwardsBlock(juce::AudioBuffer<float> &dryBuffer);

    void prepareToPlay(double sampleRate, int samplesPerBlock);

private:
    juce::AudioProcessorValueTreeState &treeState;

    juce::AudioParameterFloat *attackKnob = nullptr;
    juce::AudioParameterFloat *releaseKnob = nullptr;
    juce::AudioParameterFloat *thresholdKnob = nullptr;
    juce::AudioParameterFloat *ratioKnob = nullptr;
    juce::AudioParameterFloat *makeupKnob = nullptr;

    float ratio = 1.f;
    float threshold = 0.f;
    float kneeWidth = 0.f;

    float makeup_dB = 0.f;

    EnvelopeFollower compressorEnv;
    EnvelopeFollower expanderEnv;
};