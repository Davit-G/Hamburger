#pragma once

#include <JuceHeader.h>
#include "BlackmanWindow.h"

#include <math.h>       /* fmod */


class HilbertShifter
{
public:
    HilbertShifter();
    ~HilbertShifter();
    
    void setFrequencyShift(float newShift);
    
    void prepareToPlay(float sampleRate, int newBlockSize);
    void processBlock(float* inputBlock, int numSamples, bool tr);
    
private:
    int m_blockSize;
    float m_sampleRate {44100};
    float m_frequencyShift {0};
    
    std::vector<float> m_filter1DelayLine;
    std::vector<float> m_filter2DelayLine;
    
    
    std::vector<float> m_hilbert_B1_FilterCoefs;
    std::vector<float> m_hilbert_B2_FilterCoefs;
    
    const int numberFilter1Coefs { 31 };
    const int numberFilter2Coefs { 16 };

    float phase = 0.0f;


    
};