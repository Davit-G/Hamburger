#pragma once

#include <JuceHeader.h>

#include "./AllPassChain.h"

/*
AllPassChain
Reverb
Comb / Comb parallel
*/

class PreDistortion
{
public:
    PreDistortion(juce::AudioProcessorValueTreeState &state) : treeStateRef(state) {
        distoType = dynamic_cast<juce::AudioParameterChoice *>(state.getParameter("preDistortionType")); jassert(distoType);

        preDistortionEnabled = dynamic_cast<juce::AudioParameterBool *>(state.getParameter("preDistortionEnabled")); jassert(preDistortionEnabled);

        allPassFreq = dynamic_cast<juce::AudioParameterFloat *>(state.getParameter("allPassFreq")); jassert(allPassFreq);
        allPassQ = dynamic_cast<juce::AudioParameterFloat *>(state.getParameter("allPassQ")); jassert(allPassQ);
        allPassAmount = dynamic_cast<juce::AudioParameterFloat *>(state.getParameter("allPassAmount")); jassert(allPassAmount);

        allPassChain = std::make_unique<AllPassChain>(allPassFreq, allPassQ, allPassAmount);
    }
    ~PreDistortion() {}

    void processBlock(dsp::AudioBlock<float>& block) {
        int distoTypeIndex = distoType->getIndex();

        if (preDistortionEnabled->get() == false) return;

        switch (distoTypeIndex)
        {
        case 0: // AllPass Chain
            allPassChain->processBlock(block);
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
        allPassChain->prepareToPlay(sampleRate, samplesPerBlock);
    }

    void setSampleRate(double newSampleRate) { 
        sampleRate = newSampleRate;
    }

private:
    juce::AudioProcessorValueTreeState &treeStateRef;
    juce::AudioParameterChoice *distoType = nullptr;

    std::unique_ptr<AllPassChain> allPassChain;
    juce::AudioParameterFloat* allPassFreq = nullptr;
    juce::AudioParameterFloat* allPassQ = nullptr;
    juce::AudioParameterFloat* allPassAmount = nullptr;

    juce::AudioParameterBool* preDistortionEnabled = nullptr;

    double sampleRate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PreDistortion)
};