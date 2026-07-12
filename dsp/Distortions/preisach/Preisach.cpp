#include "Preisach.h"

#include <algorithm>
#include <cmath>
#include "../../../utils/Params.h"

Preisach::Preisach(juce::AudioProcessorValueTreeState& treeState) :
    drive(treeState, ParamIDs::preisachDrive),
    coercivity(treeState, ParamIDs::preisachCoercivity),
    remanence(treeState, ParamIDs::preisachRemanence) {}

Preisach::~Preisach() {
}

void Preisach::prepare(juce::dsp::ProcessSpec& spec) {
    stack_M_L.clear();
    stack_m_L.clear();
    stack_M_R.clear();
    stack_m_R.clear();

    stack_M_L.reserve(100);
    stack_m_L.reserve(100);
    stack_M_R.reserve(100);
    stack_m_R.reserve(100);

    stack_m_L.push_back(-1.0f);
    stack_m_R.push_back(-1.0f);

    isRisingL = true;
    isRisingR = true;
    lastSignalL = 0.0f;
    lastSignalR = 0.0f;

    cachedHistoricalAreaL = 0.0f;
    cachedHistoricalAreaR = 0.0f;

    drive.prepare(spec);
    remanence.prepare(spec);
    coercivity.prepare(spec);
}

template <typename T>
int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

float fastTanh(float x) {
    // range is -5 to 5

    int val = static_cast<int>(x * 0.1666);
    int sign = sgn(val);

    switch (val) {
        case (0): {
            // use approximation
            return juce::dsp::FastMathApproximations::tanh(x);
        }
        default: {
            // return sign
            return static_cast<float>(sign);
        }
    }
}

// computes the change in magnetisation between a certain value of alpha, beta for a distribution
// original distribution is using sech^2 instead of gaussian
float Preisach::getAnalyticalArea(float alpha, float beta, float drive, float coercivity, float remanence) const {
    const float f_alpha = fastTanh(drive * alpha);
    const float f_beta  = fastTanh(drive * beta);
    const float reversible = f_alpha - f_beta;

    const float g_alpha = fastTanh(coercivity * alpha);
    const float g_beta  = fastTanh(coercivity * beta);
    
    const float g_alpha_beta = g_alpha - g_beta;

    const float irreversible = remanence * g_alpha_beta * g_alpha_beta;

    return reversible + irreversible;
}

void Preisach::processBlock(juce::dsp::AudioBlock<float> &block) {
    drive.update();
    remanence.update();
    coercivity.update();
    
    float currentDrive = drive.getRaw(0);
    float currentCoercivity = coercivity.getRaw(0) * 2;
    float currentRemanence = remanence.getRaw(0) * 2;
    
    bool parameterChanged = drive.isChanged() || coercivity.isChanged() || remanence.isChanged();

    const int numChannels = block.getNumChannels();
    const int numSamples = block.getNumSamples();
    
    // used for normalisation
    float dynamicEMax = getAnalyticalArea(1.0f, -1.0f, currentDrive, currentCoercivity, currentRemanence);
    if (dynamicEMax <= 0.0f) dynamicEMax = 1.0f;

    for (int ch = 0; ch < numChannels; ++ch) {
        float* channelData = block.getChannelPointer(ch);

        std::vector<double>& stackM = (ch == 0) ? stack_M_L : stack_M_R;
        std::vector<double>& stackm = (ch == 0) ? stack_m_L : stack_m_R;
        bool& isRising = (ch == 0) ? isRisingL : isRisingR;
        double& lastSignal = (ch == 0) ? lastSignalL : lastSignalR;
        float& cachedArea = (ch == 0) ? cachedHistoricalAreaL : cachedHistoricalAreaR;
        
        // once per block per channel, compute runnning sum in case parameters change
        // might be expensive if block size is 1
        if (parameterChanged) {
            cachedArea = 0.0f;
            for (size_t k = 0; k < stackM.size(); ++k) {
                if (k < stackm.size()) {
                    cachedArea += getAnalyticalArea(stackM[k], stackm[k], currentDrive, currentCoercivity, currentRemanence);
                }
                if (k + 1 < stackm.size()) {
                    cachedArea -= getAnalyticalArea(stackM[k], stackm[k + 1], currentDrive, currentCoercivity, currentRemanence);
                }
            }
        }

        for (int i = 0; i < numSamples; ++i) {
            float x = channelData[i];

            if (!(x >= -100000.0f && x <= 100000.0f)) x = 0.0f;
            if (std::abs(x) < 1e-15f) x = 0.0f;
            
            const bool nowRising = (x >= lastSignal);
            if (nowRising != isRising) {
                if (isRising) { 
                    float peak = lastSignal;
                    cachedArea += getAnalyticalArea(peak, stackm.back(), currentDrive, currentCoercivity, currentRemanence);
                    stackM.push_back(peak);
                } else { 
                    float trough = lastSignal;
                    float peakForTrough = stackM.empty() ? 1.0f : stackM.back();
                    cachedArea -= getAnalyticalArea(peakForTrough, trough, currentDrive, currentCoercivity, currentRemanence);
                    stackm.push_back(trough);
                }
                isRising = nowRising;
            }

            // wipeout
            if (isRising) {
                while (!stackM.empty() && x >= stackM.back()) {
                    if (stackm.size() > 1) {
                        cachedArea += getAnalyticalArea(stackM.back(), stackm.back(), currentDrive, currentCoercivity, currentRemanence);
                        stackm.pop_back();
                    }
                    float troughForPeak = stackm.back(); 
                    cachedArea -= getAnalyticalArea(stackM.back(), troughForPeak, currentDrive, currentCoercivity, currentRemanence);
                    stackM.pop_back();
                }
            } else {
                while (stackm.size() > 1 && x <= stackm.back()) {
                    if (!stackM.empty()) {
                        float troughForPeak = stackm.back();
                        cachedArea -= getAnalyticalArea(stackM.back(), troughForPeak, currentDrive, currentCoercivity, currentRemanence);
                        stackM.pop_back();
                    }
                    float peakForTrough = stackM.empty() ? 1.0f : stackM.back();
                    cachedArea += getAnalyticalArea(peakForTrough, stackm.back(), currentDrive, currentCoercivity, currentRemanence);
                    stackm.pop_back();
                }
            }
            
            if (stackM.empty()) cachedArea = 0.0f;
            
            // trim array if we're reaching limit
            if (stackM.size() > 100 && stackm.size() > 1) {
                cachedArea += getAnalyticalArea(stackM.back(), stackm.back(), currentDrive, currentCoercivity, currentRemanence);
                stackm.pop_back();
                float troughForPeak = stackm.back();
                cachedArea -= getAnalyticalArea(stackM.back(), troughForPeak, currentDrive, currentCoercivity, currentRemanence);
                stackM.pop_back();
            }
            
            float accumulatedArea = cachedArea;
            if (isRising) {
                accumulatedArea += getAnalyticalArea(x, stackm.back(), currentDrive, currentCoercivity, currentRemanence);
            } else {
                float peakForx = stackM.empty() ? 1.0f : stackM.back();
                accumulatedArea -= getAnalyticalArea(peakForx, x, currentDrive, currentCoercivity, currentRemanence);
            }

            float finalOutput = -1.0f + (2.0f * accumulatedArea / dynamicEMax);
            channelData[i] = std::tanh(finalOutput);
            
            lastSignal = x;
        }
    }
}