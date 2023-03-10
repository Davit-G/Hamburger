

#pragma once

#include <JuceHeader.h>

#include "HilbertShifterAlt.h"
#include "BlackmanWindow.h"

#include "HilbertBiquad.h"

class FrequencyShifterAlt
{
public:
    FrequencyShifterAlt(){

    };
    ~FrequencyShifterAlt(){};

    void prepareToPlay(double sampleRate, int samplesPerBlock)
    {
        dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = samplesPerBlock;
        spec.numChannels = 2;


        m_sampleRate = sampleRate;
    };

    void processBlock(dsp::AudioBlock<float> &block)
    {
        phaseIncrement = freqShiftHz * (1 / m_sampleRate);
        

        for (int i = 0; i < block.getNumSamples(); i++)
        {

            HilbertTransformOutput hilbertLeft = hilberFreqShifterL.processSample(block.getSample(0, i));
            HilbertTransformOutput hilbertRight = hilberFreqShifterR.processSample(block.getSample(1, i));
            float QL = hilbertLeft.x * 0.5;
            float IL = hilbertLeft.y* 0.5;

            float QR = hilbertRight.x* 0.5;
            float IR = hilbertRight.y* 0.5;

            phase = fmodf(phase + phaseIncrement, 1.f);
            float theta = 2 * juce::float_Pi * phase; // this seems to be the variable responsible for the oscillator shift

            block.setSample(0, i, 2*(QL * cos(theta) - IL * sin(theta)));
            block.setSample(1, i, 2*(QR * cos(theta) - IR * sin(theta)));

            // block.setSample(0, i, QL);
            // block.setSample(1, i, IL);
        };
    };

    // void processBlock2(dsp::AudioBlock<float> &block)
    // {
    //     for (int i = 0; i < block.getNumSamples(); i++)
    //     {
    //         phaseIncrement = freqShiftHz * (1 / m_sampleRate);

    //         m_hilberFreqShifterL.processSample(block.getSample(0, i) * blackman_window_512[i]);
    //         m_hilberFreqShifterR.processSample(block.getSample(1, i) * blackman_window_512[i]);

    //         float IL = m_hilberFreqShifterL.getI();
    //         float QL = m_hilberFreqShifterL.getQ();

    //         float IR = m_hilberFreqShifterR.getI();
    //         float QR = m_hilberFreqShifterR.getQ();

    //         float theta = 2 * juce::float_Pi * (i * phaseIncrement);

    //         block.setSample(0, i, IL * std::cos(theta) - QL * std::sin(theta));
    //         block.setSample(1, i, IR * std::cos(theta) - QR * std::sin(theta));
    //     };
    // };

    void setFrequencyShift(float newFrequencyShift)
    {
        freqShiftHz = newFrequencyShift;
    };

private:
    //==============================================================================

    float m_dryWetMix{1.f};

    HilbertBiquadTransform hilberFreqShifterL;
    HilbertBiquadTransform hilberFreqShifterR;

    float freqShiftHz = 0.0f;
    double m_sampleRate;

    float phaseIncrement = 0.0f;
    float phase = 0.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrequencyShifterAlt)
};
