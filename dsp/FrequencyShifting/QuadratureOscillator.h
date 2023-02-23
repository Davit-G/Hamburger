#pragma once

#include <JuceHeader.h>



class QuadratureOscillator
{
public:
    QuadratureOscillator()
    {
        
    }

    // set frequency before running
    void prepareToPlay(double sampleRate, int samplesPerBlock) {
        // Set the phase increment based on the desired frequency and the sample rate
        m_frequency = sampleRate * 0.25;
        m_phaseIncrement = 2.0 * juce::MathConstants<float>::pi * m_frequency / sampleRate;
        m_sampleRate = sampleRate;

    }

    void setFrequencyShift(float frequency)
    {
        m_frequencyOffset = frequency;
        m_phaseIncrement = 2.0 * juce::MathConstants<float>::pi * (m_frequency + m_frequencyOffset) / m_sampleRate;
    }

    void setPhase(float phase)
    {
        m_phase = phase;
    }

    void calculateNextSample()
    {
        // Compute the sine and cosine components of the oscillator
        sine = std::sin(m_phase);
        cosine = std::cos(m_phase);

        // Update the phase of the oscillator
        m_phase += m_phaseIncrement;

        // Wrap the phase to [0, 2pi)
        if (m_phase >= 2.0 * juce::MathConstants<float>::pi)
        {
            m_phase -= 2.0 * juce::MathConstants<float>::pi;
        }
    }

    float getSine() {
        return sine;
    }

    float getCosine() {
        return cosine;
    }

private:

    


    float m_sampleRate;
    float m_frequency;
    float m_frequencyOffset = 0.0f;
    float m_phase = 0.f;
    float m_phaseIncrement;

    float sine = 0.0f;
    float cosine = 1.0f;
};
