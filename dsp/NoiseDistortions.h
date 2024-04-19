#pragma once


#include "./Noise/Sizzle.h"
#include "./Noise/Erosion.h"
#include "./Noise/Redux.h"
#include "./Distortions/HarshGate.h"

#include "SmoothParam.h"

class NoiseDistortions
{
public:
    NoiseDistortions(juce::AudioProcessorValueTreeState &state)
    {
        distoType = dynamic_cast<juce::AudioParameterChoice *>(state.getParameter("noiseDistortionType"));
        jassert(distoType);

        noiseEnabled = dynamic_cast<juce::AudioParameterBool *>(state.getParameter("noiseDistortionEnabled"));
        jassert(noiseEnabled);

        sizzle = std::make_unique<Sizzle>(state);
        erosion = std::make_unique<Erosion>(state);
        redux = std::make_unique<Redux>(state);
        gate = std::make_unique<HarshGate>(state);
        
    }
    ~NoiseDistortions(){}

    void processBlock(dsp::AudioBlock<float> &block)
    {
        int distoTypeIndex = distoType->getIndex();

        if (noiseEnabled->get() != true) return;

        switch (distoTypeIndex)
        {
        case 0: // sizzle
            { TRACE_EVENT("dsp", "sizzle");
            sizzle->processBlock(block);
            break;
            }
        case 1: // erosion
            { TRACE_EVENT("dsp", "erosion");
            erosion->processBlock(block);
            break;
            }
        case 2: // downsampling
            { TRACE_EVENT("dsp", "redux");
            redux->processBlock(block);
            break;
            }
        case 3: // jeff
            { TRACE_EVENT("dsp", "gate");
            gate->processBlock(block);
            break;
            }
        case 4: {
            sizzle->processBlockOG(block);
        }
        default:
            break;
        }
    }

    void prepare(dsp::ProcessSpec& spec)
    {
        sizzle->prepare(spec);
        erosion->prepare(spec);
        redux->prepare(spec);
    }

    void setSampleRate(float newSampleRate)
    {
        sampleRate = newSampleRate;
    }

private:
    // juce::AudioProcessorValueTreeState &treeStateRef;
    juce::AudioParameterChoice *distoType = nullptr;

    juce::AudioParameterBool* noiseEnabled;

    std::unique_ptr<Sizzle> sizzle = nullptr;
    std::unique_ptr<Erosion> erosion = nullptr;
    std::unique_ptr<Redux> redux = nullptr;
    std::unique_ptr<HarshGate> gate = nullptr;

    float sampleRate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoiseDistortions)
};