#pragma once

#include <JuceHeader.h>

#include "Distortions/SoftClipper.h"
#include "Distortions/HardClip.h"
#include "Distortions/Fuzz.h"
#include "Distortions/Cooked.h"
#include "Distortions/Jeff.h"


/*
Primary distortion modes:

Soft clip
Hard clip
Fold
Fuzz
Tube
*/

class PrimaryDistortion
{
public:
    PrimaryDistortion(juce::AudioProcessorValueTreeState &state) : treeStateRef(state)
    {
        distoType = dynamic_cast<juce::AudioParameterChoice *>(state.getParameter("primaryDistortionType"));
        jassert(distoType);

        saturationAmount = dynamic_cast<juce::AudioParameterFloat *>(state.getParameter("saturationAmount"));
        jassert(saturationAmount); 

        softClipper = std::make_unique<SoftClip>(saturationAmount);
        hardClipper = std::make_unique<HardClip>(saturationAmount);
        fold = std::make_unique<Cooked>(saturationAmount);
        fuzz = std::make_unique<Fuzz>(saturationAmount);
    };
    ~PrimaryDistortion(){};

    void processBlock(dsp::AudioBlock<float> &block)
    {
        int distoTypeIndex = distoType->getIndex();

        switch (distoTypeIndex)
        {
        case 0: // soft clipper
            softClipper->processBlock(block);
            break;
        case 1: // hard clipper
            hardClipper->processBlock(block);
            break;
        case 2: // fold
            fold->processBlock(block);
            break;
        case 3: // fuzz
            fuzz->processBlock(block);
            break;
        case 4: // tube
            break;
        case 5: 
            break;
        default:
            break;
        }
    };
    void prepareToPlay(double sampleRate, int samplesPerBlock)
    {
        softClipper->prepareToPlay(sampleRate, samplesPerBlock);
        hardClipper->prepareToPlay(sampleRate, samplesPerBlock);
        fold->prepareToPlay(sampleRate, samplesPerBlock);
        fuzz->prepareToPlay(sampleRate, samplesPerBlock);
    };

    void setSampleRate(double newSampleRate)
    {
        sampleRate = newSampleRate;
    };

private:
    juce::AudioProcessorValueTreeState &treeStateRef;
    juce::AudioParameterChoice *distoType = nullptr;

    juce::AudioParameterFloat* saturationAmount;

    std::unique_ptr<SoftClip> softClipper = nullptr;
    std::unique_ptr<HardClip> hardClipper = nullptr;
    std::unique_ptr<Cooked> fold = nullptr;
    std::unique_ptr<Fuzz> fuzz = nullptr;
    

    double sampleRate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrimaryDistortion)
};