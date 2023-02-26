#pragma once

#include <JuceHeader.h>

#include "./PreReverb.h"
#include "./CombFilter.h"
#include "./AllPassChain.h"

/*
Pre distortion modes:

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

        reverbMix = dynamic_cast<juce::AudioParameterFloat *>(state.getParameter("reverbMix")); jassert(reverbMix);
        reverbSize = dynamic_cast<juce::AudioParameterFloat *>(state.getParameter("reverbSize")); jassert(reverbSize);
        reverbWidth = dynamic_cast<juce::AudioParameterFloat *>(state.getParameter("reverbWidth")); jassert(reverbWidth);
        reverbDamping = dynamic_cast<juce::AudioParameterFloat *>(state.getParameter("reverbDamping")); jassert(reverbDamping);

        combDelay = dynamic_cast<juce::AudioParameterFloat *>(state.getParameter("combDelay")); jassert(combDelay);
        combFeedback = dynamic_cast<juce::AudioParameterFloat *>(state.getParameter("combFeedback")); jassert(combFeedback);
        combMix = dynamic_cast<juce::AudioParameterFloat *>(state.getParameter("combMix")); jassert(combMix);

        allPassFreq = dynamic_cast<juce::AudioParameterFloat *>(state.getParameter("allPassFreq")); jassert(allPassFreq);
        allPassQ = dynamic_cast<juce::AudioParameterFloat *>(state.getParameter("allPassQ")); jassert(allPassQ);
        allPassAmount = dynamic_cast<juce::AudioParameterFloat *>(state.getParameter("allPassAmount")); jassert(allPassAmount);

        preReverb = std::make_unique<PreReverb>(reverbMix, reverbSize, reverbWidth, reverbDamping);
        combFilter = std::make_unique<CombFilter>(combDelay, combFeedback, combMix);
        allPassChain = std::make_unique<AllPassChain>(allPassFreq, allPassQ, allPassAmount);
    };
    ~PreDistortion() {};

    void processBlock(dsp::AudioBlock<float>& block) {
        int distoTypeIndex = distoType->getIndex();

        if (preDistortionEnabled->get() == false) return;

        switch (distoTypeIndex)
        {
        case 0: // AllPass Chain
            allPassChain->processBlock(block);
            break;
        case 1: // Reverb
            preReverb->processBlock(block);
            break;
        case 2: // Comb / Comb parallel
            combFilter->processBlock(block);
            break;
        case 3: // 
            break;
        default:
            break;
        }
    };

    void prepareToPlay(double sampleRate, int samplesPerBlock) {
        preReverb->prepareToPlay(sampleRate, samplesPerBlock);
        combFilter->prepareToPlay(sampleRate, samplesPerBlock);
        allPassChain->prepareToPlay(sampleRate, samplesPerBlock);
    };

    void setSampleRate(double newSampleRate) { 
        sampleRate = newSampleRate;

    };

private:

    juce::AudioProcessorValueTreeState &treeStateRef;
    juce::AudioParameterChoice *distoType = nullptr;

    std::unique_ptr<PreReverb> preReverb;
    juce::AudioParameterFloat* reverbMix;
    juce::AudioParameterFloat* reverbSize;
    juce::AudioParameterFloat* reverbWidth;
    juce::AudioParameterFloat* reverbDamping;

    std::unique_ptr<CombFilter> combFilter;
    juce::AudioParameterFloat* combDelay = nullptr;
    juce::AudioParameterFloat* combFeedback = nullptr;
    juce::AudioParameterFloat* combMix = nullptr;

    std::unique_ptr<AllPassChain> allPassChain;
    juce::AudioParameterFloat* allPassFreq = nullptr;
    juce::AudioParameterFloat* allPassQ = nullptr;
    juce::AudioParameterFloat* allPassAmount = nullptr;

    juce::AudioParameterBool* preDistortionEnabled = nullptr;

    double sampleRate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PreDistortion)
};