// not currently using this class
#include "Preisach.h"

#include <algorithm>
#include <cmath>

Preisach::Preisach(juce::AudioProcessorValueTreeState& treeState, int num_hysterons, double sigma, double g) {
    this->numHysterons = num_hysterons;
    preisach_table.assign(num_hysterons, std::vector<double>(num_hysterons, 0.0f));

    if (num_hysterons <= 1) {
        eMax = 1.0f;
        return;
    }

    const double step_size = 2.0f / static_cast<double>(num_hysterons - 1);
    const double sigmadiv = 1.0f / (2.0f * sigma * sigma);
    const double gdiv = 1.0f / (2.0f * g * g);

    std::vector<std::vector<double>> density(num_hysterons, std::vector<double>(num_hysterons, 0.0f));

    for (int i = 0; i < num_hysterons; ++i) {
        for (int j = 0; j < num_hysterons; ++j) {
            const double alpha = -1.0f + static_cast<double>(i) * step_size;
            const double beta = -1.0f + static_cast<double>(j) * step_size;

            if (alpha >= beta) {
                const double exponent = -((alpha + beta) * (alpha + beta)) * sigmadiv
                    - ((alpha - beta) * (alpha - beta)) * gdiv;
                density[i][j] = std::exp(exponent);
            }
        }
    }

    for (int diff = 0; diff < num_hysterons; ++diff) {
        for (int j = 0; j < num_hysterons - diff; ++j) {
            const int i = j + diff;

            const double tLeft = (i - 1 >= j) ? preisach_table[i - 1][j] : 0.0f;
            const double tBottom = (i >= j + 1) ? preisach_table[i][j + 1] : 0.0f;
            const double tDiag = (i - 1 >= j + 1) ? preisach_table[i - 1][j + 1] : 0.0f;

            preisach_table[i][j] = tLeft + tBottom - tDiag + density[i][j];
        }
    }

    eMax = preisach_table[num_hysterons - 1][0];
    if (eMax == 0.0f) {
        eMax = 1.0f;
    }
}

Preisach::~Preisach() {
}

void Preisach::prepare(juce::dsp::ProcessSpec& spec) {
    stack_M_L.clear();
    stack_m_L.clear();
    stack_M_R.clear();
    stack_m_R.clear();

    stack_M_L.reserve(maxHistory);
    stack_m_L.reserve(maxHistory);
    stack_M_R.reserve(maxHistory);
    stack_m_R.reserve(maxHistory);

    stack_m_L.push_back(-1.0f);
    stack_m_R.push_back(-1.0f);

    isRisingL = true;
    isRisingR = true;
    lastSignalL = 0.0f;
    lastSignalR = 0.0f;

    cachedHistoricalAreaL = 0.0f;
    cachedHistoricalAreaR = 0.0f;
}

double Preisach::getInterpolatedArea(double alpha, double beta) const {
    if (numHysterons <= 1) {
        return 0.0;
    }

    const double upperBound = static_cast<double>(numHysterons - 1);
    const double idxAlpha = std::max(0.0, std::min(upperBound,
        (alpha + 1.0) * 0.5 * upperBound));
    const double idxBeta = std::max(0.0, std::min(upperBound,
        (beta + 1.0) * 0.5 * upperBound));

    const int i = static_cast<int>(idxAlpha);
    const int j = static_cast<int>(idxBeta);
    const double fracA = idxAlpha - static_cast<double>(i);
    const double fracB = idxBeta - static_cast<double>(j);

    const int iNext = std::min(i + 1, numHysterons - 1);
    const int jNext = std::min(j + 1, numHysterons - 1);

    const double t00 = preisach_table[i][j];
    const double t10 = preisach_table[iNext][j];
    const double t01 = preisach_table[i][jNext];
    const double t11 = preisach_table[iNext][jNext];

    const double bottomMix = t00 * (1.0f - fracA) + t10 * fracA;
    const double topMix = t01 * (1.0f - fracA) + t11 * fracA;
    return bottomMix * (1.0f - fracB) + topMix * fracB;
}

void Preisach::processBlock(juce::dsp::AudioBlock<float> &block) {
    if (preisach_table.empty() || preisach_table[0].empty() || eMax <= 0.0) {
        return;
    }

    const int numChannels = block.getNumChannels();
    const int numSamples = block.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch) {
        float* channelData = block.getChannelPointer(ch);

        std::vector<double>& stackM = (ch == 0) ? stack_M_L : stack_M_R;
        std::vector<double>& stackm = (ch == 0) ? stack_m_L : stack_m_R;
        bool& isRising = (ch == 0) ? isRisingL : isRisingR;
        double& lastSignal = (ch == 0) ? lastSignalL : lastSignalR;

        for (int i = 0; i < numSamples; ++i) {
            
            float x = channelData[i];
            if (!(x >= -100000.0f && x <= 100000.0f)) x = 0.0f;
            if (std::abs(x) < 1e-15f) x = 0.0f;

            float& cachedArea = (ch == 0) ? cachedHistoricalAreaL : cachedHistoricalAreaR;

            const bool nowRising = (x >= lastSignal);
            if (nowRising != isRising) {
                if (isRising) { 
                    float peak = lastSignal;
                    cachedArea += getInterpolatedArea(peak, stackm.back());
                    stackM.push_back(peak);
                } else { 
                    float trough = lastSignal;
                    float peakForTrough = stackM.empty() ? 1.0f : stackM.back();
                    cachedArea -= getInterpolatedArea(peakForTrough, trough);
                    stackm.push_back(trough);
                }
                isRising = nowRising;
            }

            if (isRising) {
                while (!stackM.empty() && x >= stackM.back()) {
                    if (stackm.size() > 1) {
                        cachedArea += getInterpolatedArea(stackM.back(), stackm.back());
                        stackm.pop_back();
                    }
                    float troughForPeak = stackm.back(); 
                    cachedArea -= getInterpolatedArea(stackM.back(), troughForPeak);
                    stackM.pop_back();
                }
            } else {
                while (stackm.size() > 1 && x <= stackm.back()) {
                    if (!stackM.empty()) {
                        float troughForPeak = stackm.back();
                        cachedArea -= getInterpolatedArea(stackM.back(), troughForPeak);
                        stackM.pop_back();
                    }
                    float peakForTrough = stackM.empty() ? 1.0f : stackM.back();
                    cachedArea += getInterpolatedArea(peakForTrough, stackm.back());
                    stackm.pop_back();
                }
            }

            if (stackM.empty()) {
                cachedArea = 0.0f;
            }

            if (stackM.size() > 100 && stackm.size() > 1) {
                cachedArea += getInterpolatedArea(stackM.back(), stackm.back());
                stackm.pop_back();
                float troughForPeak = stackm.back();
                cachedArea -= getInterpolatedArea(stackM.back(), troughForPeak);
                stackM.pop_back();
            }

            float accumulatedArea = cachedArea;
            if (isRising) {
                accumulatedArea += getInterpolatedArea(x, stackm.back());
            } else {
                float peakForx = stackM.empty() ? 1.0f : stackM.back();
                accumulatedArea -= getInterpolatedArea(peakForx, x);
            }

            const float hysteresisOutput = -1.0f + (2.0f * accumulatedArea / eMax);
            const float reversibleMix = 0.25f; 
            
            float finalOutput = (x * reversibleMix) + hysteresisOutput;
            
            channelData[i] = std::tanh(finalOutput);
            
            lastSignal = x;
        }
    }
}