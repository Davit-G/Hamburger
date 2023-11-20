#pragma once

#include "../EnvelopeFollower.h"

#include <JuceHeader.h>

class MBComp
{
public:
    MBComp(juce::AudioProcessorValueTreeState &state) : compressor1(state, CompressionType::UPWARDS_DOWNWARDS),
                                                        compressor2(state, CompressionType::UPWARDS_DOWNWARDS),
                                                        compressor3(state, CompressionType::UPWARDS_DOWNWARDS),
                                                        threshold(state, "compThreshold"),
                                                        ratio(state, "compRatio"),
                                                        attack(state, "compAttack"),
                                                        release(state, "compRelease"),
                                                        makeup(state, "compOut") {}

    ~MBComp() {}

    void processBlock(dsp::AudioBlock<float> &block)
    {
        attack.update();
        release.update();
        makeup.update();
        ratio.update();
        threshold.update();

        block.copyTo(lowBuffer);
        block.copyTo(midBuffer);
        block.copyTo(highBuffer);

        // low result
        lowCrossOver.process(dsp::ProcessContextReplacing<float>(lowBlock)); // initial low end
        midBlock.addProductOf(lowBlock, -1.f); // get everything except low end, give to mid block
        highCrossOver.process(dsp::ProcessContextReplacing<float>(midBlock)); // get high end from mid block
        highBlock.copyFrom(midBlock); // put high end into high block
        midBlock.addProductOf(highBlock, -1.f); // set proper mid freqs

        float atk = attack.get();
        float rel = release.get();
        float mkp = makeup.get();
        float rat = ratio.get();
        float thr = threshold.get();

        compressor1.updateParameters(atk, rel, mkp, rat, thr + 2.f, 0.1f, 0.f);
        compressor2.updateParameters(atk, rel, mkp, rat, thr, 0.1f, 0.f);
        compressor3.updateParameters(atk, rel, mkp, rat, thr, 0.1f, 0.f);
        
        compressor1.processBlock(lowBlock);
        compressor2.processBlock(midBlock);
        compressor3.processBlock(highBlock);

        block.copyFrom(lowBlock);
        block.addProductOf(midBlock, 1.0f);
        block.addProductOf(highBlock, 1.0f);
    }

    void prepare(dsp::ProcessSpec &spec)
    {
        compressor1.prepare(spec);
        compressor2.prepare(spec);
        compressor3.prepare(spec);

        lowCrossOver.setCutoffFrequency(500.0f);
        highCrossOver.setCutoffFrequency(2500.0f);
        lowCrossOver.setType(dsp::LinkwitzRileyFilterType::lowpass);
        highCrossOver.setType(dsp::LinkwitzRileyFilterType::highpass);
        lowCrossOver.prepare(spec);
        highCrossOver.prepare(spec);

        lowBuffer.setSize(2, spec.maximumBlockSize);
        midBuffer.setSize(2, spec.maximumBlockSize);
        highBuffer.setSize(2, spec.maximumBlockSize);

        lowBlock = dsp::AudioBlock<float>(lowBuffer);
        midBlock = dsp::AudioBlock<float>(midBuffer);
        highBlock = dsp::AudioBlock<float>(highBuffer);
    }

private:
    double sampleRate;

    SmoothParam threshold;
    SmoothParam ratio;
    SmoothParam attack;
    SmoothParam release;
    SmoothParam makeup;

    AudioBuffer<float> lowBuffer;
    AudioBuffer<float> midBuffer;
    AudioBuffer<float> highBuffer;
    dsp::AudioBlock<float> lowBlock;
    dsp::AudioBlock<float> midBlock;
    dsp::AudioBlock<float> highBlock;

    // mid-high 2.5khz
    // low - mid 500hz

    dsp::LinkwitzRileyFilter<float> lowCrossOver;
    dsp::LinkwitzRileyFilter<float> highCrossOver;

    Compressor compressor1;
    Compressor compressor2;
    Compressor compressor3;
};