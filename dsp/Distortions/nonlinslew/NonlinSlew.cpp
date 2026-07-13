#include "NonlinSlew.h"

#include <algorithm>
#include <cmath>

namespace {

template <typename T>
int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

float safePow(float value, float exponent) {
    const float magnitude = std::max(std::fabs(value), 1.0e-6f);
    return std::pow(magnitude, exponent);
}

} // namespace

void NonlinSlew::resetState() {
    lastSampleL = 0.0f;
    lastSampleR = 0.0f;
    ema_L = 0.0f;
    ema_R = 0.0f;
}

void NonlinSlew::prepare(juce::dsp::ProcessSpec& spec) {
    emaParam.prepare(spec);
    alphaParam.prepare(spec);
    slewSpeed.prepare(spec);
    directionality.prepare(spec);
    lastMode = -1;
    resetState();
}

void NonlinSlew::processBlock(juce::dsp::AudioBlock<float> &block) {
    emaParam.update();
    alphaParam.update();
    slewSpeed.update();
    directionality.update();

    int mode = type->get();
    if (mode != lastMode) {
        resetState();
        lastMode = mode;
    }
    
    switch (mode) {
        case 0: {
            // tanh based jumps
            for (int i = 0; i < block.getNumSamples(); i++) {
                float inL = block.getSample(0, i);
                float inR = block.getSample(1, i);
        
                float deltaL = inL - lastSampleL;
                float deltaR = inR - lastSampleR;
        
                int signL = sgn(deltaL);
                int signR = sgn(deltaR);
        
                float alpha = safePow(alphaParam.getNextValue(0), 0.3f);
                float ema_a = safePow(emaParam.getNextValue(0), 3.0f) * 50.0f;
                float slewSpeedVal = slewSpeed.getNextValue(0);
                float slewScaled = safePow(std::fabs(slewSpeedVal), 3.0f) * (slewSpeedVal < 0.0f ? -1.0f : 1.0f);
                float bend = directionality.getNextValue(0) * 4.0f;
        
                float jumpDistL = (tanh(deltaL * slewScaled * 50.0f + bend) - tanh(bend)) * slewScaled;
                float jumpDistR = (tanh(deltaR * slewScaled * 50.0f + bend) - tanh(bend)) * slewScaled;
        
                lastSampleL = (1.0f - alpha) * inL + alpha * std::tanh(lastSampleL + signL * deltaL * jumpDistL);
                lastSampleR = (1.0f - alpha) * inR + alpha * std::tanh(lastSampleR + signR * deltaR * jumpDistR);
        
                if (!std::isfinite(lastSampleL)) lastSampleL = 0.0f;
                if (!std::isfinite(lastSampleR)) lastSampleR = 0.0f;

                block.setSample(0, i, lastSampleL);
                block.setSample(1, i, lastSampleR);
            }
        
            break;
        }
        case 1: {
            // gaussian / strange attractor?
            for (int i = 0; i < block.getNumSamples(); i++) {
                float inL = block.getSample(0, i);
                float inR = block.getSample(1, i);
        
                float deltaL = inL - lastSampleL;
                float deltaR = inR - lastSampleR;
                
                float drive = safePow(alphaParam.getNextValue(0), 3.0f);
                float alpha = drive * 50.0f;
                float ema_a = drive;
                float slew_bias = slewSpeed.getNextValue(0);
                float bend = directionality.getNextValue(0);

                float bL = (deltaL + ema_L * bend);
                float bR = (deltaR + ema_R * bend);
        
                float jumpDistL = exp(-alpha * bL * bL) * slew_bias;
                float jumpDistR = exp(-alpha * bR * bR) * slew_bias;
        
                lastSampleL = std::tanh(lastSampleL + deltaL * jumpDistL);
                lastSampleR = std::tanh(lastSampleR + deltaR * jumpDistR);
        
                if (!std::isfinite(lastSampleL)) lastSampleL = 0.0f;
                if (!std::isfinite(lastSampleR)) lastSampleR = 0.0f;

                ema_L = ema_a * lastSampleL + (1.0f - ema_a) * ema_L;
                ema_R = ema_a * lastSampleR + (1.0f - ema_a) * ema_R;
                
                if (!std::isfinite(ema_L)) ema_L = 0.0f;
                if (!std::isfinite(ema_R)) ema_R = 0.0f;

                block.setSample(0, i, lastSampleL);
                block.setSample(1, i, lastSampleR);
            }
            
            break;
        }



    }
}