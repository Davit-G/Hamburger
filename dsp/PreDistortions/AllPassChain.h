#pragma once

constexpr auto registerSize = dsp::SIMDRegister<float>::size();

class AllPassChain
{
public:
    AllPassChain(juce::AudioProcessorValueTreeState& treeState) : 
        allPassFrequency(treeState, "allPassFreq"), 
        allPassQ(treeState, "allPassQ"), 
        allPassAmount(treeState, "allPassAmount") {}

    ~AllPassChain() {}

    template <typename SampleType>
    auto prepareChannelPointers (const dsp::AudioBlock<SampleType>& block)
    {
        // pads the input channels with zero buffers if the number of channels is less than the register size
        std::array<SampleType*, registerSize> result {};

        for (size_t ch = 0; ch < result.size(); ++ch)
            result[ch] = (ch < block.getNumChannels() ? block.getChannelPointer (ch) : zero.getChannelPointer (ch));

        return result;
    }

    void processBlock(dsp::AudioBlock<float> &block)
    {
        TRACE_EVENT("dsp", "AllPassChain::processBlock");
        
        allPassAmount.update();
        allPassFrequency.update();
        allPassQ.update();

        float amount = allPassAmount.getRaw();
        float freq = allPassFrequency.getRaw();
        float q = allPassQ.getRaw();

        // interleaved = dsp::AudioBlock<dsp::SIMDRegister<float>>(interleavedBlockData, 1, block.getNumSamples());
        auto interleavedSubBlock = interleaved.getSubBlock(0, block.getNumSamples());
        auto context = dsp::ProcessContextReplacing<float>(block); // maybe doesnt need to be created every time?? idk


        const auto& input  = context.getInputBlock();
        auto numSamples = (int) block.getNumSamples();
        
        auto inChannels = prepareChannelPointers (input);
        
        if (!((oldAllPassFreq == freq) && (oldAllPassQ == q) && (oldAllPassAmount == amount))) {
            for (int i = 0; i < amount; i++) {
                *iir[i].state = dsp::IIR::ArrayCoefficients<float>::makeAllPass(oldSampleRate, allPassFrequency.getRaw(), allPassQ.getRaw());
            }
            oldAllPassFreq = freq;
            oldAllPassQ = q;
            oldAllPassAmount = amount;
        }

        AudioData::interleaveSamples(AudioData::NonInterleavedSource<Format> { inChannels.data(),                                 registerSize, },
                                    AudioData::InterleavedDest<Format>      { toBasePointer (interleavedSubBlock.getChannelPointer (0)), registerSize },
                                    numSamples);
        
        dsp::ProcessContextReplacing<dsp::SIMDRegister<float>> newCtx (interleavedSubBlock);
        
        for (size_t i = 0; i < amount; i++) {
            iir[i].process(newCtx);
        }

        auto outChannels = prepareChannelPointers (context.getOutputBlock());

        AudioData::deinterleaveSamples(AudioData::InterleavedSource<Format>  { toBasePointer (interleavedSubBlock.getChannelPointer (0)), registerSize },
                                        AudioData::NonInterleavedDest<Format> { outChannels.data(),                                registerSize },
                                        numSamples);
    }

    void prepare(dsp::ProcessSpec& spec)
    {
        allPassFrequency.prepare(spec);
        allPassQ.prepare(spec);
        allPassAmount.prepare(spec);

        // new SIMD stuff
        for (int i = 0; i < 50; i++) {
            // iir[i].reset(new dsp::IIR::Filter<dsp::SIMDRegister<float>>(iirCoefficients));
            iir[i].reset();
            *iir[i].state = dsp::IIR::ArrayCoefficients<float>::makeAllPass(spec.sampleRate, allPassFrequency.getRaw(), allPassQ.getRaw());;
        }

        interleaved = dsp::AudioBlock<dsp::SIMDRegister<float>>(interleavedBlockData, 1, spec.maximumBlockSize);
        zero = dsp::AudioBlock<float>(zeroData, dsp::SIMDRegister<float>::size(), spec.maximumBlockSize);

        zero.clear();

        auto monoSpec = spec;
        monoSpec.numChannels = 1;
        
        for (int i = 0; i < 50; i++) {
            // iir[i]->prepare(monoSpec);
            iir[i].prepare(monoSpec);
        }

        oldSampleRate = spec.sampleRate;
    }

private:
    SmoothParam allPassFrequency;
    SmoothParam allPassQ;
    SmoothParam allPassAmount;

    template <typename T>
    static T *toBasePointer(dsp::SIMDRegister<T> *r) noexcept { return reinterpret_cast<T *>(r); }
    using Format = AudioData::Format<AudioData::Float32, AudioData::NativeEndian>;

    float oldAllPassFreq = 0.0;
    float oldAllPassQ = 0.0;
    float oldAllPassAmount = 0.0;

    float oldSampleRate;

    dsp::ProcessorDuplicator<dsp::IIR::Filter<dsp::SIMDRegister<float>>, dsp::IIR::Coefficients<float>> iir[50];

    dsp::AudioBlock<dsp::SIMDRegister<float>> interleaved;
    dsp::AudioBlock<float> zero;

    HeapBlock<char> interleavedBlockData, zeroData;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AllPassChain)
};