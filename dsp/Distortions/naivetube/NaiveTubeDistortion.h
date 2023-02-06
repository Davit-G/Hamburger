#pragma once

#include <JuceHeader.h>
#include "fxobjects/fxobjects.h"

class NaiveTubeDistortion
{
public:
    NaiveTubeDistortion(juce::AudioProcessorValueTreeState &state);
    ~NaiveTubeDistortion();

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();

    void processBlock(juce::AudioBuffer<float> &buffer);

    void setDrive(float drive);
    void setTone(float tone);
private:
    ClassATubePre preL;
    ClassATubePre preR;
    ClassATubePreParameters preParameters;

    juce::AudioProcessorValueTreeState &treeStateRef;
	juce::AudioParameterFloat *knobValue = nullptr;

    float drive;
    float tone;
};