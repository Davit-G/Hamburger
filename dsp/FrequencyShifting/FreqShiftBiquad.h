#pragma once

#include <JuceHeader.h>

class FrequencyShifter
{
public:
    FrequencyShifter()
    {
        // *iirFilter[0].state = *dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 880.0, 3.0);
        // *iirFilter[1].state = *dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, 880.0, 3.0);
    }
    
    void processBlock(dsp::AudioBlock<float>& block)
    {
        float phase = 0.0f;
        float phaseIncrement = shiftAmount * MathConstants<float>::twoPi / sampleRate;
        
        for (int i = 0; i < block.getNumSamples(); i++)
        {
            float inputSampleL = block.getSample(0, i);
            float inputSampleR = block.getSample(1, i);
            
            // Apply Hilbert transform
            // dsp::Complex<float> complexSample = { inputSample, 0.0f };
            hilbertTransformL.processSample(inputSampleL);
            hilbertTransformR.processSample(inputSampleR);
            
            // Calculate cosine and sine signals
            float cosValL = hilbertTransformL.getQ(); // HERE IS WHERE YOU SUPPOSEDLY FILTER
            float sinValL = hilbertTransformL.getI(); // HERE IS WHERE YOU SUPPOSEDLY FILTER
            
            // Multiply cosine and sine signals by appropriate phase and sum
            block.setSample(0, i, 2*(cos(phase) * cosValL - sin(phase) * sinValL));

            // Calculate cosine and sine signals
            float cosValR = hilbertTransformR.getQ(); // HERE IS WHERE YOU SUPPOSEDLY FILTER
            float sinValR = hilbertTransformR.getI(); // HERE IS WHERE YOU SUPPOSEDLY FILTER
            
            // Multiply cosine and sine signals by appropriate phase and sum
            block.setSample(1, i, 2*(cos(phase) * cosValR - sin(phase) * sinValR));

            phase += phaseIncrement;
            if (phase > MathConstants<float>::twoPi)
                phase -= MathConstants<float>::twoPi;
        }
    }
    
    void setSampleRate(float newSampleRate)
    {
        sampleRate = newSampleRate;
        // realFilter.reset();
        // imagFilter.reset();
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) {
        this->sampleRate = sampleRate;
    }

    void setFrequencyShift(float newFrequencyShift) {
        shiftAmount = newFrequencyShift;
    }
    
private:
    float sampleRate = 44100.0f;
    float shiftAmount = 0.0f;


    HilbertTransform hilbertTransformL;
    HilbertTransform hilbertTransformR;
};

class HilbertTransform
{
public:
    HilbertTransform() :
        z1_1(0), z1_2(0), z1_3(0), z1_4(0),
        z2_1(0), z2_2(0), z2_3(0),
        a1_1(0.6923878), a1_2(0.9360654322959),
        a1_3(0.9882295226860), a1_4(0.9987488452737),
        a2_1(0.4021921162426), a2_2(0.8561710882420),
        a2_3(0.9722909545651), a2_4(0.9952884791278)
    {}

    float getI(){return I;}
    float getQ(){return Q;}

    void processSample(float x)
    {
        float y1 = a1_1 * (x - z1_1) + z1_2;
        float y2 = a1_2 * (y1 - z1_2) + z1_3;
        float y3 = a1_3 * (y2 - z1_3) + z1_4;
        float y4 = a1_4 * (y3 - z1_4);
        z1_1 = x;
        z1_2 = y1;
        z1_3 = y2;
        z1_4 = y3;

        float w1 = a2_1 * (x - z2_1) + z2_2;
        float w2 = a2_2 * (w1 - z2_2) + z2_3;
        float w3 = a2_3 * (w2 - z2_3);
        z2_1 = x;
        z2_2 = w1;
        z2_3 = w2;

        I = 0.5 * (y4 + w3);
        Q = 0.5 * (y4 - w3);
    }

private:
    float z1_1, z1_2, z1_3, z1_4;
    float z2_1, z2_2, z2_3;
    const float a1_1, a1_2, a1_3, a1_4;
    const float a2_1, a2_2, a2_3, a2_4;
    

    float I;
    float Q;
};
