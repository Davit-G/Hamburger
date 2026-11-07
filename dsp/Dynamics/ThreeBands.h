#pragma once

#include "../Crossover.h"
#include "../LinearPhaseSplit.h"

/*  Lows, mids and highs for the multiband compressor and transient shaper. Either way they sum back to the input, through
    the crossovers' allpasses or delayed by the linear phase split, which is what the dry gets matched to. */
class ThreeBands
{
public:
    static constexpr std::array<float, 2> cutoffs { 200.0f, 3000.0f };
    static constexpr int slope = 24;

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        for (auto* crossover : { &lowSplit, &highSplit, &lowAllpass })
        {
            crossover->prepare (spec);
            crossover->setSlope (slope);
        }

        lowSplit.setCutoffFrequency (cutoffs[0]);
        highSplit.setCutoffFrequency (cutoffs[1]);
        lowAllpass.setCutoffFrequency (cutoffs[1]);

        linearSplit.prepare (spec);
        linearSplit.setCutoffs (cutoffs.data(), (int) cutoffs.size());

        for (auto& band : bands)
            band.setSize ((int) spec.numChannels, (int) spec.maximumBlockSize, false, false, true);
    }

    void setLinear (bool shouldBeLinear) { linear = shouldBeLinear; }
    bool isLinear() const { return linear; }

    void split (const juce::dsp::AudioBlock<float>& block)
    {
        if (linear)
            return linearSplit.split (block, bands.data(), (int) cutoffs.size());

        // both channels inside the sample loop, see Crossover::processSample
        for (int n = 0; n < (int) block.getNumSamples(); ++n)
        {
            for (int ch = 0; ch < (int) block.getNumChannels(); ++ch)
            {
                float low = 0.0f, rest = 0.0f, mid = 0.0f, high = 0.0f;

                lowSplit.processSample (ch, block.getSample (ch, n), low, rest);
                highSplit.processSample (ch, rest, mid, high);

                // the lows never went through the high split, so they get its allpass to sum flat with the rest
                bands[0].setSample (ch, n, lowAllpass.allpass (ch, low));
                bands[1].setSample (ch, n, mid);
                bands[2].setSample (ch, n, high);
            }
        }
    }

    // for the types that don't split, to keep the same latency as the ones that do
    void delayBy (juce::dsp::AudioBlock<float>& block) { linearSplit.delayBy (block); }

    int getLatency() const { return linearSplit.getLatency(); }

    static Crossovers crossovers() { return { slope, (int) cutoffs.size(), { cutoffs[0], cutoffs[1] } }; }

    std::array<juce::AudioBuffer<float>, 3> bands;

private:
    Crossover lowSplit, highSplit, lowAllpass;
    LinearPhaseSplit linearSplit;
    bool linear = false;
};
