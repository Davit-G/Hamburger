#include <JuceHeader.h>

#include "./Noise/Sizzle.h"
#include "./Noise/Erosion.h"
#include "./Noise/Redux.h"
#include "./Noise/Jeff.h"

#include "SmoothParam.h"

class NoiseDistortions
{
public:
    NoiseDistortions(juce::AudioProcessorValueTreeState &state) : treeStateRef(state)
    {
        distoType = dynamic_cast<juce::AudioParameterChoice *>(state.getParameter("noiseDistortionType"));
        jassert(distoType);

        noiseEnabled = dynamic_cast<juce::AudioParameterBool *>(state.getParameter("noiseDistortionEnabled"));
        jassert(noiseEnabled);

        sizzle = std::make_unique<Sizzle>(state);
        erosion = std::make_unique<Erosion>(state);
        redux = std::make_unique<Redux>(state);
        jeff = std::make_unique<Jeff>(state);
        
    }
    ~NoiseDistortions(){}

    void processBlock(dsp::AudioBlock<float> &block)
    {
        int distoTypeIndex = distoType->getIndex();

        if (noiseEnabled->get() != true) return;

        switch (distoTypeIndex)
        {
        case 0: // sizzle
            sizzle->processBlock(block);
            break;
        case 1: // erosion
            erosion->processBlock(block, sampleRate);
            break;
        case 2: // downsampling
            redux->processBlock(block);
            break;
        case 3: // smooth downsampling
            redux->antiAliasingStep(block);
            redux->processBlock(block);
            break;
        case 4: // asperity
            jeff->processBlock(block);
            break;
        default:
            break;
        }
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock)
    {
        sizzle->prepareToPlay(sampleRate, samplesPerBlock);
        erosion->prepareToPlay(sampleRate, samplesPerBlock);
        jeff->prepareToPlay(sampleRate, samplesPerBlock);
        redux->prepareToPlay(sampleRate, samplesPerBlock);
    }

    void setSampleRate(double newSampleRate)
    {
        sampleRate = newSampleRate;
    }

private:
    juce::AudioProcessorValueTreeState &treeStateRef;
    juce::AudioParameterChoice *distoType = nullptr;

    juce::AudioParameterBool* noiseEnabled;

    std::unique_ptr<Sizzle> sizzle = nullptr;
    std::unique_ptr<Erosion> erosion = nullptr;
    std::unique_ptr<Redux> redux = nullptr;
    std::unique_ptr<Jeff> jeff = nullptr;

    double sampleRate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoiseDistortions)
};