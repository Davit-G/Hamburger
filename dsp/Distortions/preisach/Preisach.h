#pragma once

#include <vector>
#include "juce_dsp/juce_dsp.h"
#include "juce_audio_processors/juce_audio_processors.h"
#include "../../SmoothParam.h"

#define PREISACH_USING_SIMD 1

class Preisach {
public:
    Preisach(juce::AudioProcessorValueTreeState& treeState);
    ~Preisach();

    void prepare(juce::dsp::ProcessSpec& spec);
    void processBlock(juce::dsp::AudioBlock<float> &block);

    
private:
    float getAnalyticalArea(float alpha, float beta, float drive, float coercivity, float remanence) const;

    static constexpr int maxHistory = 512;
    
    std::vector<double> stack_M_L = std::vector<double>(maxHistory);
    std::vector<double> stack_m_L = std::vector<double>(maxHistory);
    std::vector<double> stack_M_R = std::vector<double>(maxHistory);
    std::vector<double> stack_m_R = std::vector<double>(maxHistory);

    bool isRisingL = true;
    bool isRisingR = true;
    double lastSignalL = 0.0f;
    double lastSignalR = 0.0f;

    float cachedHistoricalAreaL = 0.0f;
    float cachedHistoricalAreaR = 0.0f;

    float dynamicEMax = 0.0f;

    SmoothParam drive;
    SmoothParam bias;
    SmoothParam remanence;

    float possibleMaxValue = 5.0f;

    double getInterpolatedArea(double alpha, double beta) const;
};