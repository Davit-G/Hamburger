// not currently using this class

#pragma once

#include <vector>
#include "juce_dsp/juce_dsp.h"
#include "juce_audio_processors/juce_audio_processors.h"

class Preisach {
public:
    Preisach(juce::AudioProcessorValueTreeState& treeState, int num_hysterons, double sigma, double g);
    ~Preisach();

    void prepare(juce::dsp::ProcessSpec& spec);
    void processBlock(juce::dsp::AudioBlock<float> &block);

private:
    // Preisach precomputation table
    std::vector<std::vector<double>> preisach_table;

    double eMax = 1.0f;
    int numHysterons = 0;

    static constexpr int maxHistory = 100;

    std::vector<double> stack_M_L;
    std::vector<double> stack_m_L;
    std::vector<double> stack_M_R;
    std::vector<double> stack_m_R;

    bool isRisingL = true;
    bool isRisingR = true;
    double lastSignalL = 0.0f;
    double lastSignalR = 0.0f;

    float cachedHistoricalAreaL = 0.0f;
    float cachedHistoricalAreaR = 0.0f;

    double getInterpolatedArea(double alpha, double beta) const;
};