#pragma once

#include <JuceHeader.h>


class Dynamics
{
public:
    Dynamics(juce::AudioProcessorValueTreeState &state) : treeStateRef(state) {
        distoType = dynamic_cast<juce::AudioParameterChoice *>(state.getParameter("compressionType")); jassert(distoType);
        enabled = dynamic_cast<juce::AudioParameterBool *>(state.getParameter("compressionOn")); jassert(enabled);
    }
    ~Dynamics() {}

    void processBlock(dsp::AudioBlock<float>& block) {
        int distoTypeIndex = distoType->getIndex();

        if (!enabled->get()) return;

        switch (distoTypeIndex)
        {
        case 0:
            break;
        case 1:
            break;
        case 2:
            break;
        case 3:
            break;
        default:
            break;
        }
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) {
    }

    void setSampleRate(double newSampleRate) { 
        sampleRate = newSampleRate;
    }

private:
    juce::AudioProcessorValueTreeState &treeStateRef;
    juce::AudioParameterChoice *distoType = nullptr;

    juce::AudioParameterBool* enabled = nullptr;

    double sampleRate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Dynamics)
};