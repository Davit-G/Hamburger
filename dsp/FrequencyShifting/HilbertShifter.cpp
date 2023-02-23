
#include "HilbertShifter.h"

HilbertShifter::HilbertShifter()
{
    m_filter1DelayLine.resize(numberFilter1Coefs);
    m_hilbert_B1_FilterCoefs.resize(numberFilter1Coefs);
    m_hilbert_B1_FilterCoefs = {0.f, 0.f, -0.0008f, 0.f, -0.0045f, 0.f, -0.0142f, 0.f, -0.0358f, 0.f, -0.0802f, 0.f, -0.1802f, 0.f, -0.6253f, 0.f, 0.6253f, 0.f, 0.1802f, 0.f, 0.0802f, 0.f, 0.0358f, 0.f, 0.0142f, 0.f, 0.0045f, 0.f, 0.0008f, 0.f, 0.f};

    m_filter2DelayLine.resize(numberFilter2Coefs);
    ;
    m_hilbert_B2_FilterCoefs.resize(numberFilter2Coefs);
    m_hilbert_B2_FilterCoefs = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f};
}

HilbertShifter::~HilbertShifter()
{
    m_hilbert_B1_FilterCoefs.clear();
    m_hilbert_B2_FilterCoefs.clear();
}

void HilbertShifter::setFrequencyShift(float newShift)
{
    m_frequencyShift = newShift;
}

void HilbertShifter::prepareToPlay(float sampleRate, int newBlockSize)
{
    m_sampleRate = sampleRate;
    m_blockSize = newBlockSize;
}

void HilbertShifter::processBlock(float *inputBlock, int numSamples, bool tr)
{   
    float phaseIncrement = m_frequencyShift * (1 / m_sampleRate);
    for (int i = 0; i < numSamples; ++i)
    {
        float windowedSample = inputBlock[i] * blackman_window_512[i];

        float I = windowedSample * m_hilbert_B1_FilterCoefs[0];
        for (int b = 0; b < numberFilter1Coefs - 1 && b < m_filter1DelayLine.size(); ++b)
        {
            I += m_hilbert_B1_FilterCoefs[b + 1] * m_filter1DelayLine[m_filter1DelayLine.size() - b - 1];
        }

        float Q = windowedSample * m_hilbert_B2_FilterCoefs[0];
        for (int b = 0; b < numberFilter2Coefs - 1 && b < m_filter2DelayLine.size(); ++b)
        {
            Q += m_hilbert_B2_FilterCoefs[b + 1] * m_filter2DelayLine[m_filter2DelayLine.size() - b - 1];
        }

        phase = fmodf(phase + phaseIncrement, 1.f);
        float theta = 2 * juce::float_Pi * phase; // u seems to be the variable responsible for the phase shift or something.
        inputBlock[i] = 2*(I * std::cos(theta) - Q * sin(theta));

        std::rotate(m_filter1DelayLine.begin(), m_filter1DelayLine.begin() + 1, m_filter1DelayLine.end());
        std::rotate(m_filter2DelayLine.begin(), m_filter2DelayLine.begin() + 1, m_filter2DelayLine.end());

        m_filter1DelayLine[m_filter1DelayLine.size() - 1] = windowedSample;
        m_filter2DelayLine[m_filter2DelayLine.size() - 1] = windowedSample;
    }
}