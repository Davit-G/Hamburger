#pragma once

#include <JuceHeader.h>

#include "Distortions/SoftClipper.h"
#include "Distortions/HardClip.h"
#include "Distortions/Fuzz.h"
#include "Distortions/Cooked.h"
#include "Distortions/Jeff.h"
#include "Distortions/Grunge.h"


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

        distortionEnabled = dynamic_cast<juce::AudioParameterBool *>(state.getParameter("primaryDistortionEnabled"));
        jassert(distortionEnabled);

        saturationAmount = dynamic_cast<juce::AudioParameterFloat *>(state.getParameter("saturationAmount"));
        jassert(saturationAmount);

        fuzzParam = dynamic_cast<juce::AudioParameterFloat *>(state.getParameter("fuzz"));
        jassert(fuzzParam);

        foldParam = dynamic_cast<juce::AudioParameterFloat *>(state.getParameter("fold"));
        jassert(foldParam);

        bias = dynamic_cast<juce::AudioParameterFloat *>(state.getParameter("bias"));
        jassert(bias);

        grungeToneParam = dynamic_cast<juce::AudioParameterFloat *>(state.getParameter("grungeTone"));
        grungeAmountParam = dynamic_cast<juce::AudioParameterFloat *>(state.getParameter("grungeAmt"));
        jassert(grungeToneParam);
        jassert(grungeAmountParam);

        softClipper = std::make_unique<SoftClip>(saturationAmount);
        hardClipper = std::make_unique<HardClip>(saturationAmount);
        fold = std::make_unique<Cooked>(foldParam);
        fuzz = std::make_unique<Fuzz>(fuzzParam);
        grunge = std::make_unique<Grunge>(grungeAmountParam, grungeToneParam);
    }

    ~PrimaryDistortion() {}

    void processBlock(dsp::AudioBlock<float> &block)
    {
        int distoTypeIndex = distoType->getIndex();

        if (distortionEnabled->get()  == false) return;

        switch (distoTypeIndex)
        {
        case 0: // classic
            fold->processBlock(block);
            fuzz->processBlock(block);
            block.add(bias->get());
            grunge->processBlock(block);
            softClipper->processBlock(block);
            break;
        case 1: // tube
            hardClipper->processBlock(block);
            break;
        case 2: // fold
            break;
        case 3: // fuzz
            break;
        case 4: // tube
            break;
        case 5:
            break;
        default:
            break;
        }

        iirFilter.process(dsp::ProcessContextReplacing<float>(block)); // hpf afterwards to remove bias
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock)
    {
        softClipper->prepareToPlay(sampleRate, samplesPerBlock);
        hardClipper->prepareToPlay(sampleRate, samplesPerBlock);
        fold->prepareToPlay(sampleRate, samplesPerBlock);
        fuzz->prepareToPlay(sampleRate, samplesPerBlock);
        grunge->prepareToPlay(sampleRate, samplesPerBlock);

        // init iir filter
        *iirFilter.state = *dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 14.0f, 0.6);

        dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = samplesPerBlock;
        spec.numChannels = 2;

        iirFilter.prepare(spec);
    }

    void setSampleRate(double newSampleRate)
    {
        sampleRate = newSampleRate;
    }

private:
    juce::AudioProcessorValueTreeState &treeStateRef;
    juce::AudioParameterChoice *distoType = nullptr;

    juce::AudioParameterFloat *saturationAmount;
    juce::AudioParameterFloat *fuzzParam;
    juce::AudioParameterFloat *foldParam;
    juce::AudioParameterFloat *grungeToneParam;
    juce::AudioParameterFloat *grungeAmountParam;

    juce::AudioParameterBool *distortionEnabled;


    std::unique_ptr<SoftClip> softClipper = nullptr;
    std::unique_ptr<HardClip> hardClipper = nullptr;
    std::unique_ptr<Cooked> fold = nullptr;
    std::unique_ptr<Fuzz> fuzz = nullptr;
    std::unique_ptr<Grunge> grunge = nullptr;

    juce::AudioParameterFloat *bias;

    dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> iirFilter;

    double sampleRate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrimaryDistortion)
};