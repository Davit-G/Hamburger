#pragma once

#include <JuceHeader.h>
#include "Compressor.h"

#include "../SmoothParam.h"

class Dynamics
{
public:
    Dynamics(juce::AudioProcessorValueTreeState &state) : treeStateRef(state),
        compressor1(state, CompressionType::UPWARDS_DOWNWARDS),
        compressor2(state, CompressionType::UPWARDS_DOWNWARDS),
        compressor3(state, CompressionType::UPWARDS_DOWNWARDS),
        threshold(state, "compThreshold"),
        ratio(state, "compRatio"),
        attack(state, "compAttack"),
        release(state, "compRelease"),
        makeup(state, "compOut")
        // lowBlock(lowBuffer),
        // midBlock(midBuffer),
        // highBlock(highBuffer)
    {
        distoType = dynamic_cast<juce::AudioParameterChoice *>(state.getParameter("compressionType")); jassert(distoType);
        enabled = dynamic_cast<juce::AudioParameterBool *>(state.getParameter("compressionOn")); jassert(enabled);
    }
    ~Dynamics() {}

    void processBlock(dsp::AudioBlock<float>& block) {
        int distoTypeIndex = distoType->getIndex();
        attack.update();
        release.update();
        makeup.update();
        ratio.update();
        threshold.update();

        for (int sample = 0; sample < block.getNumSamples(); sample++)
        {
            float leftSample = block.getSample(0, sample);
            float rightSample = block.getSample(1, sample);

            float lowResultL = lowCrossOver.processSample(0, leftSample);
            float midResultL = leftSample - lowResultL;
            float highResultL = highCrossOver.processSample(0, midResultL);
            midResultL = midResultL - highResultL;

            float lowResultR = lowCrossOver.processSample(1, rightSample);
            float midResultR = rightSample - lowResultR;
            float highResultR = highCrossOver.processSample(1, midResultR);
            midResultR = midResultR - highResultR;

            lowBuffer.setSample(0, sample, lowResultL);
            midBuffer.setSample(0, sample, midResultL);
            highBuffer.setSample(0, sample, highResultL);
            lowBuffer.setSample(1, sample, lowResultR);
            midBuffer.setSample(1, sample, midResultR);
            highBuffer.setSample(1, sample, highResultR);
        }

        float atk = attack.get();
        float rel = release.get();
        float mkp = makeup.get();
        float rat = ratio.get();
        float thr = threshold.get();

        compressor1.updateParameters(atk, rel, mkp, rat, thr + 3.f, 0.1f, 0.f);
        compressor2.updateParameters(atk, rel, mkp, rat, thr, 0.1f, 0.f);
        compressor3.updateParameters(atk, rel, mkp, rat, thr, 0.1f, 0.f);

        dsp::AudioBlock<float> lowBlock(lowBuffer);
        dsp::AudioBlock<float> midBlock(midBuffer);
        dsp::AudioBlock<float> highBlock(highBuffer);

        if (!enabled->get()) return;

        switch (distoTypeIndex)
        {
        case 0: // "COMPANDER"
            compressor1.processBlock(lowBlock);
            compressor2.processBlock(midBlock);
            compressor3.processBlock(highBlock);
            break;
        case 1: // "OTT",
            
            break;
        case 2: // "MID-SIDE"
            
            break;
        case 3:
            
            break;
        default:
            break;
        }

        for (int sample = 0; sample < block.getNumSamples(); sample++) {
            block.setSample(0, sample, 
                lowBuffer.getSample(0, sample) + 
                midBuffer.getSample(0, sample) + 
                highBuffer.getSample(0, sample));
            block.setSample(1, sample, 
                lowBuffer.getSample(1, sample) + 
                midBuffer.getSample(1, sample) + 
                highBuffer.getSample(1, sample));
        }
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) {
        compressor1.prepareToPlay(sampleRate, samplesPerBlock);
        compressor2.prepareToPlay(sampleRate, samplesPerBlock);
        compressor3.prepareToPlay(sampleRate, samplesPerBlock);

        dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = samplesPerBlock;
        spec.numChannels = 2;

        lowCrossOver.setCutoffFrequency(500.0f);
        highCrossOver.setCutoffFrequency(2500.0f);
        lowCrossOver.setType(dsp::LinkwitzRileyFilterType::lowpass);
        highCrossOver.setType(dsp::LinkwitzRileyFilterType::highpass);
        lowCrossOver.prepare(spec);
        highCrossOver.prepare(spec);

        lowBuffer.setSize(2, samplesPerBlock);
        midBuffer.setSize(2, samplesPerBlock);
        highBuffer.setSize(2, samplesPerBlock);
    }

    void setSampleRate(double newSampleRate) { 
        sampleRate = newSampleRate;
    }

private:
    juce::AudioProcessorValueTreeState &treeStateRef;
    juce::AudioParameterChoice *distoType = nullptr;

    juce::AudioParameterBool* enabled = nullptr;

    double sampleRate;

    SmoothParam threshold;
    SmoothParam ratio;
    SmoothParam attack;
    SmoothParam release;
    SmoothParam makeup;

    AudioBuffer<float> lowBuffer;
    AudioBuffer<float> midBuffer;
    AudioBuffer<float> highBuffer;
    // dsp::AudioBlock<float> lowBlock;
    // dsp::AudioBlock<float> midBlock;
    // dsp::AudioBlock<float> highBlock;

    // mid-high 2.5khz
    // low - mid 500hz

    dsp::LinkwitzRileyFilter<float> lowCrossOver;
    dsp::LinkwitzRileyFilter<float> highCrossOver;

    Compressor compressor1;
    Compressor compressor2;
    Compressor compressor3;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Dynamics)
};