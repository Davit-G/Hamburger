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
    for (int i = 0; i < lastSampleBuf.size(); i++) {
        lastSampleBuf[i] = 0.0f;
        emaBuf[i] = 0.0f;    
    }
}

void NonlinSlew::prepare(juce::dsp::ProcessSpec& spec) {
    emaParam.prepare(spec);
    alphaParam.prepare(spec);
    slewSpeed.prepare(spec);
    directionality.prepare(spec);
    lastMode = -1;
    lastSampleBuf.resize(spec.numChannels);
    emaBuf.resize(spec.numChannels);

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
    
    for (int ch = 0; ch < block.getNumChannels(); ch++) {
        float* data = block.getChannelPointer(ch);
        float& lastSample = lastSampleBuf[ch];
        float& ema = emaBuf[ch];

        switch (mode) {
            case 0: {
                // tanh based jumps
                for (int i = 0; i < block.getNumSamples(); i++) {
                    const float in = data[i];

                    float delta = in - lastSample;
            
                    int sign = sgn(delta);
            
                    float alpha = safePow(alphaParam.getNextValue(ch), 0.3f);
                    float ema_a = safePow(emaParam.getNextValue(ch), 3.0f) * 50.0f;
                    float slewSpeedVal = slewSpeed.getNextValue(ch);
                    float slewScaled = safePow(std::fabs(slewSpeedVal), 3.0f) * (slewSpeedVal < 0.0f ? -1.0f : 1.0f);
                    float bend = directionality.getNextValue(ch) * 4.0f;
            
                    float jumpDist = (tanh(delta * slewScaled * 50.0f + bend) - tanh(bend)) * slewScaled;
            
                    lastSample = (1.0f - alpha) * in + alpha * std::tanh(lastSample + sign * delta * jumpDist);
            
                    if (!std::isfinite(lastSample)) lastSample = 0.0f;
    
                    data[i] = lastSample;
                }
            
                break;
            }
            case 1: {
                // gaussian / strange attractor?
                for (int i = 0; i < block.getNumSamples(); i++) {
                    const float in = data[i];
            
                    float delta = in - lastSample;
                    
                    float drive = safePow(alphaParam.getNextValue(0), 3.0f);
                    float alpha = drive * 50.0f;
                    float ema_a = drive;
                    float slew_bias = slewSpeed.getNextValue(0);
                    float bend = directionality.getNextValue(0);
    
                    float b = (delta + ema * bend);
                    float jumpDist = exp(-alpha * b * b) * slew_bias;
            
                    lastSample = std::tanh(lastSample + delta * jumpDist);
            
                    if (!std::isfinite(lastSample)) lastSample = 0.0f;
    
                    ema = ema_a * lastSample + (1.0f - ema_a) * ema;
                    if (!std::isfinite(ema)) ema = 0.0f;
    
                    data[i] = lastSample;
                }
                
                break;
            }
    
    
    
        }

    }
}