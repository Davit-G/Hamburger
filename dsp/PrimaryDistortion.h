#pragma once

#include <JuceHeader.h>

#include "Distortions/SoftClipper.h"
#include "Distortions/HardClip.h"
#include "Distortions/Fuzz.h"
#include "Distortions/Cooked.h"
#include "Distortions/Grunge.h"

#include "Distortions/tube/Amp.h"

class PrimaryDistortion
{
public:
    PrimaryDistortion(juce::AudioProcessorValueTreeState &state) : treeStateRef(state),
        bias(state, "bias")
    {   
        distoType = dynamic_cast<juce::AudioParameterChoice *>(state.getParameter("primaryDistortionType"));
        jassert(distoType);

        distortionEnabled = dynamic_cast<juce::AudioParameterBool *>(state.getParameter("primaryDistortionEnabled"));
        jassert(distortionEnabled);

        softClipper = std::make_unique<SoftClip>(state);
        hardClipper = std::make_unique<HardClip>(state);
        fold = std::make_unique<Cooked>(state);
        fuzz = std::make_unique<Fuzz>(state);
        grunge = std::make_unique<Grunge>(state);
        tubeAmp = std::make_unique<Amp>(state);
    }

    ~PrimaryDistortion() {}

    void processBlock(dsp::AudioBlock<float> &block)
    {
        int distoTypeIndex = distoType->getIndex();

        bias.update();

        if (distortionEnabled->get()  == false) return;

        switch (distoTypeIndex)
        {
        case 0: // classic
            // fold->processBlock(block);
            fuzz->processBlock(block);

            for (int i = 0; i < block.getNumSamples(); i++)
            {
                auto biasAmt = bias.get();
                block.setSample(0, i, block.getSample(0, i) + biasAmt);
                block.setSample(1, i, block.getSample(1, i) + biasAmt);
            }
            
            grunge->processBlock(block);
            softClipper->processBlock(block);
            iirFilter.process(dsp::ProcessContextReplacing<float>(block)); // hpf afterwards to remove bias
            break;
        case 1: // tube
            tubeAmp->processBlock(block);
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
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock)
    {
        softClipper->prepareToPlay(sampleRate, samplesPerBlock);
        hardClipper->prepareToPlay(sampleRate, samplesPerBlock);
        fold->prepareToPlay(sampleRate, samplesPerBlock);
        fuzz->prepareToPlay(sampleRate, samplesPerBlock);
        grunge->prepareToPlay(sampleRate, samplesPerBlock);
        tubeAmp->prepareToPlay(sampleRate, samplesPerBlock);

        // init iir filter
        *iirFilter.state = *dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 20.0f, 1.0f);

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
    juce::AudioParameterBool *distortionEnabled;

    std::unique_ptr<SoftClip> softClipper = nullptr;
    std::unique_ptr<HardClip> hardClipper = nullptr;
    std::unique_ptr<Cooked> fold = nullptr;
    std::unique_ptr<Fuzz> fuzz = nullptr;
    std::unique_ptr<Grunge> grunge = nullptr;
    std::unique_ptr<Amp> tubeAmp = nullptr;

    SmoothParam bias;

    dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> iirFilter;

    double sampleRate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrimaryDistortion)
};