#include "Preisach.h"

#include <algorithm>
#include <cmath>
#include "../../../utils/Params.h"

Preisach::Preisach(juce::AudioProcessorValueTreeState& treeState) :
    drive(treeState, ParamIDs::tapeDrive),
    bias(treeState, ParamIDs::tapeBias),
    remanence(treeState, ParamIDs::tapeWidth) {}

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

    drive.prepare(spec);
    remanence.prepare(spec);
    bias.prepare(spec);
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
    // const float f_alpha = fastTanh(drive * alpha);
    // const float f_beta  = fastTanh(drive * beta);
    // const float reversible = f_alpha - f_beta;

    // const float g_alpha = fastTanh(drive * alpha);
    // const float g_beta  = fastTanh(drive * beta);
    
    // const float g_alpha_beta = g_alpha - g_beta;

    // const float irreversible = 2.0f * remanence * g_alpha_beta * g_alpha_beta;
    const float f_alpha = fastTanh(drive * alpha);
    const float f_beta  = fastTanh(drive * beta);
    const float reversible = f_alpha - f_beta;

    const float irreversible = 2.0f * remanence * reversible * reversible;

    return reversible + irreversible;
}

void Preisach::processBlock(juce::dsp::AudioBlock<float> &block) {
    drive.update();
    remanence.update();
    bias.update();
    
    float driveRaw = drive.getCurrent(0);
    float driveSquared = driveRaw * driveRaw;
    float gainMult = powf(1.0f - driveSquared, 3.0f) + 1.0f;

    float currentDrive = driveSquared * 6.0f + 0.5f;
    float currentCoercivity = bias.getCurrent(0) * 2.0f;
    
    float rem = remanence.getCurrent(0);
    float currentRemanence = (rem * rem) * 2.0f + 0.000001f;
    
    bool parameterChanged = drive.isChanged() || bias.isChanged() || remanence.isChanged();

    const int numChannels = block.getNumChannels();
    const int numSamples = block.getNumSamples();
    
    // used for normalisation
    
    for (int ch = 0; ch < numChannels; ++ch) {
        float* channelData = block.getChannelPointer(ch);
        
        std::vector<double>& stackM = (ch == 0) ? stack_M_L : stack_M_R;
        std::vector<double>& stackm = (ch == 0) ? stack_m_L : stack_m_R;
        bool& isRising = (ch == 0) ? isRisingL : isRisingR;
        double& lastSignal = (ch == 0) ? lastSignalL : lastSignalR;
        float& cachedArea = (ch == 0) ? cachedHistoricalAreaL : cachedHistoricalAreaR;
        
        for (int i = 0; i < numSamples; ++i) {
        // once per block per channel, compute runnning sum in case parameters change
        // might be expensive if block size is 1
        // cachedArea = 0.0f;
        // for (size_t k = 0; k < stackM.size(); ++k) {
        //     if (k < stackm.size()) {
        //         cachedArea += getAnalyticalArea(stackM[k], stackm[k], currentDrive, currentCoercivity, currentRemanence);
        //     }
        //     if (k + 1 < stackm.size()) {
        //         cachedArea -= getAnalyticalArea(stackM[k], stackm[k + 1], currentDrive, currentCoercivity, currentRemanence);
        //     }
        // }
        
            float x = channelData[i];
            
            driveRaw = drive.getNextValue(ch);
            driveSquared = driveRaw * driveRaw;
            gainMult = powf(1.0f - driveSquared, 3.0f) + 1.0f;
            currentDrive = driveSquared * 6.0f + 0.5f;
            currentCoercivity = bias.getNextValue(ch) * 2.0f;
            rem = remanence.getNextValue(ch);
            currentRemanence = (rem * rem) * 2.0f + 0.000001f;

            float center = getAnalyticalArea(0.0f, 1.0f, currentDrive, currentCoercivity, currentRemanence);
            
            dynamicEMax = getAnalyticalArea(possibleMaxValue, -possibleMaxValue, currentDrive, currentCoercivity, currentRemanence);
            if (dynamicEMax <= 0.0f) dynamicEMax = 1.0f;

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
            // cachedArea *= 0.9999f; // decay the cached area over time, this makes me feel a little less uneasy about this code
            
            // trim array if we're reaching limit
            if (stackM.size() > maxHistory && stackm.size() > 1) {
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

            float finalOutput = (accumulatedArea - center) / dynamicEMax;
            finalOutput *= gainMult;
            channelData[i] = finalOutput;
            
            lastSignal = x;
        }
    }
}