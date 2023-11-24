#pragma once

#include <xsimd/xsimd.hpp>

template <typename T>
struct SizeTraits {
    static int getSize() { return T::size; }
};

template <typename T>
struct SizeTraits<dsp::SIMDRegister<T>> {
    static int getSize() { return dsp::SIMDRegister<T>::size(); }
};

template <typename T>
struct SizeTraits<xsimd::batch<T>> {
    static int getSize() { return xsimd::batch<T>::size; }
};

/*
Operations to interleave, deinterleave, and convert between. Might happen a few times so might as well get it out of the way.
Accepts a SIMDType, which can be a juce::SIMDRegister<T> or xsimd::batch<T> type.

Hopefully we can interleave and deinterleave only once, and have the relevant stuff do it's processing in the interleaved domain.
*/


template <typename T, typename SIMDType>
static T *toBasePointer(SIMDType *r) noexcept { return reinterpret_cast<T *>(r); }

template <typename SIMDType, typename SampleType>
class SIMDBufferOps
{
public:

    SIMDBufferOps() {}
    ~SIMDBufferOps() {}

    int getRegisterSize() { return SizeTraits<SIMDType>::getSize(); }

    /*
    Fill empty channel slots in the SIMD register with zeroes.
    */
    auto prepareChannelPointers(const dsp::AudioBlock<SampleType> &block)
    {
        // pads the input channels with zero buffers if the number of channels is less than the register size
        std::array<SampleType *, simdRegSize> result{};

        for (size_t ch = 0; ch < result.size(); ++ch)
            result[ch] = (ch < block.getNumChannels() ? block.getChannelPointer(ch) : zero.getChannelPointer(ch));

        return result;
    }

    /*
        Prepares interleaving related stuff. 
        Returns a ProcessSpec with the number of channels set to 1, representing an interleaved buffer.
        Call prepare on any DSP units that take interleaved buffers.
    */
    dsp::ProcessSpec prepare(juce::dsp::ProcessSpec& spec) {
        interleaved = dsp::AudioBlock<SIMDType>(interleavedBlockData, 1, spec.maximumBlockSize);
        zero = dsp::AudioBlock<SampleType>(zeroData, getRegisterSize(), spec.maximumBlockSize);

        zero.clear();

        auto monoSpec = spec;
        monoSpec.numChannels = 1;

        return monoSpec;
    }

    dsp::ProcessContextReplacing<SampleType> interleaveBlock(dsp::AudioBlock<SampleType>& block) {
        interleavedSubBlock = &interleaved.getSubBlock(0, block.getNumSamples());
        auto numSamples = block.getNumSamples(); // remember this can be variable depending on host
        auto regSize = getRegisterSize();

        auto inChannels = prepareChannelPointers(block);

        AudioData::interleaveSamples(AudioData::NonInterleavedSource<Format>{
                                         inChannels.data(),
                                         regSize,
                                     },
                                     AudioData::InterleavedDest<Format>{toBasePointer(interleavedSubBlock->getChannelPointer(0)), regSize}, numSamples);

        return dsp::ProcessContextReplacing<SIMDType>(*interleavedSubBlock);
    }

    void deinterleaveBlock(dsp::AudioBlock<SampleType>& block) {
        auto numSamples = block.getNumSamples(); // remember this can be variable depending on host
        auto outChannels = prepareChannelPointers(block);
        auto regSize = getRegisterSize();

        AudioData::deinterleaveSamples(AudioData::InterleavedSource<Format>{toBasePointer(interleavedSubBlock->getChannelPointer(0)), regSize},
                                       AudioData::NonInterleavedDest<Format>{outChannels.data(), regSize},
                                       numSamples);
    }

private:
    using Format = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;

    dsp::AudioBlock<SIMDType> interleaved;
    dsp::AudioBlock<SampleType> zero;

    dsp::AudioBlock<SIMDType> *interleavedSubBlock = nullptr;

    HeapBlock<char> interleavedBlockData, zeroData;

};