#include <JuceHeader.h>
#include "LossyIntegrator.h"


void LossyIntegrator::prepare(dsp::ProcessSpec &spec)
{
    sampleRate = spec.sampleRate;
    T = 1.0f / sampleRate;
    integrator_z[0] = 0.0f;
    integrator_z[1] = 0.0f;
}

float LossyIntegrator::processAudioSample(float xn)
{
    float hpf = alpha0 * (xn - rho * integrator_z[0] - integrator_z[1]);
    float bpf = alpha * hpf + integrator_z[0];
    float lpf = alpha * bpf + integrator_z[1];

    integrator_z[0] = alpha * hpf + bpf;
    integrator_z[1] = alpha * bpf + lpf;

    return lpf;
}

void LossyIntegrator::calculateFilterCoeffs()
{
    float g = sampleRate * dsp::FastMathApproximations::tan(wd * T * 0.5f) * T;

    alpha0 = 1.0f / (1.0f + g * (2.0f * R + g));
    alpha = g;
    rho = 2.0f * R + g;
}
