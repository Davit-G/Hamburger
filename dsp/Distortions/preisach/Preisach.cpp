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

// this only computes the raw, unscaled irreversible memory shape
// used for continuous parameter update
float Preisach::getIrreversibleArea(float alpha, float beta) const {
    const float rev = fastTanh(alpha) - fastTanh(beta);
    return rev * rev; // We handle the 2.0f and Remanence at the output
}

void Preisach::processBlock(juce::dsp::AudioBlock<float> &block) {
    drive.update();
    remanence.update();
    bias.update();

    const int numChannels = block.getNumChannels();
    const int numSamples = block.getNumSamples();

    for (int ch = 0; ch < numChannels; ++ch) {
        float* channelData = block.getChannelPointer(ch);
        
        std::vector<double>& stackM = (ch == 0) ? stack_M_L : stack_M_R;
        std::vector<double>& stackm = (ch == 0) ? stack_m_L : stack_m_R;
        bool& isRising = (ch == 0) ? isRisingL : isRisingR;
        double& lastSignal = (ch == 0) ? lastSignalL : lastSignalR;
        float& cachedArea = (ch == 0) ? cachedHistoricalAreaL : cachedHistoricalAreaR;

        for (int i = 0; i < numSamples; ++i) {
            float x = channelData[i];
            
            float driveRaw = drive.getNextValue(ch);
            float driveSquared = driveRaw * driveRaw;
            float currentDrive = driveSquared * 6.0f + 0.5f;
            
            float rem = remanence.getNextValue(ch);
            float currentRemanence = (rem * rem) * 2.0f + 0.000001f;

            float drivenX = x * currentDrive;

            if (!(drivenX >= -100000.0f && drivenX <= 100000.0f)) drivenX = 0.0f;
            if (std::abs(drivenX) < 1e-15f) drivenX = 0.0f;
            
            const bool nowRising = (drivenX >= lastSignal);
            if (nowRising != isRising) {
                if (isRising) { 
                    float peak = lastSignal;
                    cachedArea += getIrreversibleArea(peak, stackm.back());
                    stackM.push_back(peak);
                } else { 
                    float trough = lastSignal;
                    float peakForTrough = stackM.empty() ? 1.0f : stackM.back();
                    cachedArea -= getIrreversibleArea(peakForTrough, trough);
                    stackm.push_back(trough);
                }
                isRising = nowRising;
            }

            // wipeout
            if (isRising) {
                while (!stackM.empty() && drivenX >= stackM.back()) {
                    if (stackm.size() > 1) {
                        cachedArea += getIrreversibleArea(stackM.back(), stackm.back());
                        stackm.pop_back();
                    }
                    float troughForPeak = stackm.back(); 
                    cachedArea -= getIrreversibleArea(stackM.back(), troughForPeak);
                    stackM.pop_back();
                }
            } else {
                while (stackm.size() > 1 && drivenX <= stackm.back()) {
                    if (!stackM.empty()) {
                        float troughForPeak = stackm.back();
                        cachedArea -= getIrreversibleArea(stackM.back(), troughForPeak);
                        stackM.pop_back();
                    }
                    float peakForTrough = stackM.empty() ? 1.0f : stackM.back();
                    cachedArea += getIrreversibleArea(peakForTrough, stackm.back());
                    stackm.pop_back();
                }
            }
            
            if (stackM.empty()) cachedArea = 0.0f;
            
            // trim array if we're reaching limit
            if (stackM.size() > maxHistory && stackm.size() > 1) {
                cachedArea += getIrreversibleArea(stackM.back(), stackm.back());
                stackm.pop_back();
                float troughForPeak = stackm.back();
                cachedArea -= getIrreversibleArea(stackM.back(), troughForPeak);
                stackM.pop_back();
            }
            
            float accumulatedIrrev = cachedArea;
            if (isRising) {
                accumulatedIrrev += getIrreversibleArea(drivenX, stackm.back());
            } else {
                float peakForx = stackM.empty() ? 1.0f : stackM.back();
                accumulatedIrrev -= getIrreversibleArea(peakForx, drivenX);
            }

            float reversiblePart = fastTanh(drivenX) - fastTanh(-possibleMaxValue); 
            float irreversiblePart = 2.0f * currentRemanence * accumulatedIrrev;

            float rawOutput = reversiblePart + irreversiblePart;

            // normalisation
            float center = fastTanh(0.0f) - fastTanh(-possibleMaxValue) + 
                           (2.0f * currentRemanence * getIrreversibleArea(0.0f, -possibleMaxValue));
                           
            float eMax = fastTanh(possibleMaxValue) - fastTanh(-possibleMaxValue) + 
                         (2.0f * currentRemanence * getIrreversibleArea(possibleMaxValue, -possibleMaxValue));
            float dynamicEMax = eMax <= 0.0f ? 1.0f : eMax;

            float finalOutput = (rawOutput - center) / dynamicEMax;

            float invDrive = 1.0f - driveSquared;
            float gainMult = invDrive * invDrive + 2.0f;
            finalOutput *= gainMult;

            channelData[i] = finalOutput;
            
            lastSignal = drivenX;
        }
    }
}