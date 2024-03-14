#pragma once

#include "xsimd/xsimd.hpp"
#include "SmoothParam.h"

class SIMDGain {
public:
    SIMDGain(juce::AudioProcessorValueTreeState& treeState)
    // : gainKnob(treeState, "saturationAmount") 
    {}

    void prepare(dsp::ProcessSpec& spec) {
        // gainKnob.prepare(spec);
        // using batch = xsimd::batch<float, xsimd::best_arch>;
        // remainderDataZeroes.setSize(2, batch::size);
    }

    struct process {

        template <class Arch>
        void operator()(Arch, dsp::AudioBlock<float>& block, float gain) {
            using batch = xsimd::batch<float, Arch>;

            juce::AudioBuffer<float> remainderDataZeroes;
            remainderDataZeroes.setSize(2, batch::size);

            auto leftChan = block.getChannelPointer(0);
            auto rightChan = block.getChannelPointer(1);

            batch gainD(gain);

            int iters = block.getNumSamples() / batch::size;
            int remainder = block.getNumSamples() & batch::size;

            // process the remainders first, we might not even have to do any number of iterations!
            if (remainder) {
                int remStartIdx = iters * batch::size;
                for (int i = 0; i < batch::size; i++) {
                    if (i > remainder) {
                        remainderDataZeroes.setSample(0, i, 0);
                        remainderDataZeroes.setSample(1, i, 0);
                    } else {
                        remainderDataZeroes.setSample(0, i, leftChan[remStartIdx + i]);
                        remainderDataZeroes.setSample(1, i, rightChan[remStartIdx + i]);
                    }
                }

                auto lPtr = remainderDataZeroes.getReadPointer(0);
                auto rPtr = remainderDataZeroes.getReadPointer(1);

                // put that all into a batch
                batch remainderDataL = xsimd::load_unaligned(lPtr);
                batch remainderDataR = xsimd::load_unaligned(rPtr);
                // do the multiply
                remainderDataL *= gainD;
                remainderDataR *= gainD;

                // put back into extra buffer
                remainderDataL.store_unaligned(remainderDataZeroes.getWritePointer(0));
                remainderDataR.store_unaligned(remainderDataZeroes.getWritePointer(0));

                // take data needed from extra buffer back into regular buffer
                for (int i = 0; i < remainder; i++) {
                    block.setSample(0, remStartIdx + i, lPtr[remStartIdx + i]);
                    block.setSample(1, remStartIdx + i, rPtr[remStartIdx + i]);
                }
            }

            for (int i = 0; i < iters; i += 1) {
                batch dataL = xsimd::load_unaligned<Arch, float>(leftChan + i * batch::size);
                batch dataR = xsimd::load_unaligned<Arch, float>(rightChan + i * batch::size);
                dataL *= gainD;
                dataR *= gainD;
                dataL.store_unaligned(leftChan + i * batch::size);
                dataR.store_unaligned(rightChan + i * batch::size);
            }

        }
    };

private:
    // SmoothParam gainKnob;
};