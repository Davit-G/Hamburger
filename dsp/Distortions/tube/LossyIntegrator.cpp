#include <JuceHeader.h>
#include "LossyIntegrator.h"


void LossyIntegrator::prepare(dsp::ProcessSpec &spec)
{
    sampleRate = spec.sampleRate;
    T = 1.0f / sampleRate;
    integrator_z[0] = 0.0f;
    integrator_z[1] = 0.0f;
}

dsp::SIMDRegister<float> LossyIntegrator::processAudioSample(dsp::SIMDRegister<float> xn)
{
    auto hpf = (xn - (integrator_z[0] * rho) - integrator_z[1]) * alpha0;
    auto bpf = hpf * alpha + integrator_z[0];
    auto lpf = bpf * alpha + integrator_z[1];

    integrator_z[0] = hpf * alpha + bpf;
    integrator_z[1] = bpf * alpha + lpf;

    return lpf;
}

float LossyIntegrator::processAudioSample(float xn) {
    auto hpf = (xn - (integrator_z[0][0] * rho) - integrator_z[1][0]) * alpha0;
    auto bpf = hpf * alpha + integrator_z[0][0];
    auto lpf = bpf * alpha + integrator_z[1][0];

    integrator_z[0][0] = hpf * alpha + bpf;
    integrator_z[1][0] = bpf * alpha + lpf;

    return lpf;
}

void LossyIntegrator::calculateFilterCoeffs()
{
    float g = sampleRate * dsp::FastMathApproximations::tan(wd * T * 0.5f) * T;

    alpha0 = 1.0f / (1.0f + g * (2.0f * R + g));
    alpha = g;
    rho = 2.0f * R + g;
}
