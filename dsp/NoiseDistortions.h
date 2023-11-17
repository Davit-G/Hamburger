#include <JuceHeader.h>

#include "./Noise/Sizzle.h"
#include "./Noise/Erosion.h"
#include "./Noise/Redux.h"
#include "./Noise/Jeff.h"

class NoiseDistortions
{
public:
    NoiseDistortions(juce::AudioProcessorValueTreeState &state) : treeStateRef(state)
    {
        distoType = dynamic_cast<juce::AudioParameterChoice *>(state.getParameter("noiseDistortionType"));
        jassert(distoType);

        noiseEnabled = dynamic_cast<juce::AudioParameterBool *>(state.getParameter("noiseDistortionEnabled"));
        jassert(noiseEnabled);

        noiseAmount = dynamic_cast<juce::AudioParameterFloat *>(state.getParameter("noiseAmount"));
        noiseFrequency = dynamic_cast<juce::AudioParameterFloat *>(state.getParameter("noiseFrequency"));
        noiseQ = dynamic_cast<juce::AudioParameterFloat *>(state.getParameter("noiseQ"));
        jassert(noiseAmount);
        jassert(noiseFrequency);
        jassert(noiseQ);

        downsampleAmount = dynamic_cast<juce::AudioParameterFloat *>(state.getParameter("downsampleAmount"));
        downsampleJitter = dynamic_cast<juce::AudioParameterFloat *>(state.getParameter("downsampleJitter"));
        bitReduction = dynamic_cast<juce::AudioParameterFloat *>(state.getParameter("bitReduction"));
        jassert(downsampleAmount);
        jassert(downsampleJitter);
        jassert(bitReduction);

        sizzle = std::make_unique<Sizzle>(noiseAmount);
        erosion = std::make_unique<Erosion>(noiseAmount, noiseFrequency, noiseQ);
        redux = std::make_unique<Redux>(downsampleAmount, downsampleJitter, bitReduction);
        jeff = std::make_unique<Jeff>(noiseAmount);
        
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
        case 3: // asperity
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
    }

    void setSampleRate(double newSampleRate)
    {
        sampleRate = newSampleRate;
    }

private:
    juce::AudioProcessorValueTreeState &treeStateRef;
    juce::AudioParameterChoice *distoType = nullptr;

    juce::AudioParameterFloat* noiseAmount;
    juce::AudioParameterFloat* noiseFrequency;
    juce::AudioParameterFloat* noiseQ;
    juce::AudioParameterFloat* downsampleAmount;
    juce::AudioParameterFloat* downsampleJitter;
    juce::AudioParameterFloat* bitReduction;

    juce::AudioParameterBool* noiseEnabled;

    std::unique_ptr<Sizzle> sizzle = nullptr;
    std::unique_ptr<Erosion> erosion = nullptr;
    std::unique_ptr<Redux> redux = nullptr;
    std::unique_ptr<Jeff> jeff = nullptr;

    double sampleRate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoiseDistortions)
};