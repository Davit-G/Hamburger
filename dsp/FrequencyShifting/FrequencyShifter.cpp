
#include "FrequencyShifter.h"

//==============================================================================
void FrequencyShifter::prepareToPlay (double sampleRate, int samplesPerBlock)
{

    dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 2;


    processedBuffer.setSize(2, samplesPerBlock);
    
    iirFilter.prepare(spec);
    *iirFilter.state = *dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 4000);
    
    m_frequencyLFO.prepare(spec);

    m_frequencyLFO.initialise([] (float x) { return std::sin (x); }, 256);
    
    
    m_hilberFreqShifterL.prepareToPlay(sampleRate, samplesPerBlock);
    m_hilberFreqShifterR.prepareToPlay(sampleRate, samplesPerBlock);
    
}


void FrequencyShifter::setDryWetMix(float newMixValue)
{
    m_dryWetMix = newMixValue;
}

void FrequencyShifter::setFrequencyShift(float newFrequencyShift)
{
    m_freqShift = newFrequencyShift;
    
    
    m_hilberFreqShifterL.setFrequencyShift(newFrequencyShift);
    m_hilberFreqShifterR.setFrequencyShift(newFrequencyShift);
    
}


void FrequencyShifter::setLFORate(float newRate)
{
    m_frequencyLFO.setFrequency(newRate);
}


void FrequencyShifter::setLFODepth(float newDepth)
{
    m_lfoDepth = newDepth;
}

void FrequencyShifter::processBlock (dsp::AudioBlock<float>& block)
{
    
    int numSamples = block.getNumSamples();
    
    processedBuffer.clear();
    for (int i = 0; i < block.getNumSamples(); i++) {
        // store the filtered random value in the buffer
        processedBuffer.setSample(0, i, block.getSample(0, i));
        processedBuffer.setSample(1, i, block.getSample(1, i));
    }


    const float* inputBufferL = block.getChannelPointer(0);
    // FloatVectorOperations::copy(processedBuffer.getWritePointer(0), inputBufferL, numSamples); 
    m_hilberFreqShifterL.processBlock(processedBuffer.getWritePointer(0), numSamples, false);

    const float* inputBufferR = block.getChannelPointer(1);
    // FloatVectorOperations::copy(processedBuffer.getWritePointer(1), inputBufferR, numSamples);
    
    m_hilberFreqShifterR.processBlock(processedBuffer.getWritePointer(1), numSamples, true);
    

    // dsp::AudioBlock<float> moddedAudio(processedBuffer);

    // iirFilter.process(dsp::ProcessContextReplacing<float>(moddedAudio));

    for (int channel = 0; channel < numStereoChannels; ++channel)
    {
        const float* shiftedBuffer = processedBuffer.getWritePointer(channel);
        for (int i=0; i<numSamples; ++i)
        {
            block.setSample(channel, i, m_dryWetMix * shiftedBuffer[i] + (1.f - m_dryWetMix) * block.getSample(channel, i));
        }
    }
    
}
