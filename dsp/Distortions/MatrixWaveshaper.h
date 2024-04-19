#pragma once

#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"

inline double tanhWaveShaper(double xn, double saturation) noexcept
{
    return tanh(saturation * xn) / tanh(saturation);
}

inline double cursedSinWS(double xn, double saturation) noexcept
{
    return xn * sin(xn * saturation) * 0.3;
}

inline double omegaN(double xn, double s) noexcept
{
    double sv = s * 50;
    return (xn + s * tanh(sv * xn)) * (1 - s * 0.5);
}

inline double shredShape(double x, double c) noexcept
{
    // crossover distort before shred
    if (abs(x) < 0.005)
        return 0;

    return c * (floor(c * x * 10) - (0.5 * floor(c * x * 20)) + 0.25) * 0.1;
}

class MatrixWaveshaper
{
public:
    MatrixWaveshaper(juce::AudioProcessorValueTreeState &treeState) : mat1Param(treeState, ParamIDs::matrix1),
                                                                      mat2Param(treeState, ParamIDs::matrix2),
                                                                      mat3Param(treeState, ParamIDs::matrix3),
                                                                      mat4Param(treeState, ParamIDs::matrix4),
                                                                      mat5Param(treeState, ParamIDs::matrix5),
                                                                      mat6Param(treeState, ParamIDs::matrix6),
                                                                      mat7Param(treeState, ParamIDs::matrix7),
                                                                      mat8Param(treeState, ParamIDs::matrix8),
                                                                      mat9Param(treeState, ParamIDs::matrix9)
    {
    }

    void prepare(juce::dsp::ProcessSpec &spec)
    {
        mat1Param.prepare(spec);
        mat2Param.prepare(spec);
        mat3Param.prepare(spec);
        mat4Param.prepare(spec);
        mat5Param.prepare(spec);
        mat6Param.prepare(spec);
        mat7Param.prepare(spec);
        mat8Param.prepare(spec);
        mat9Param.prepare(spec);
    }

    void processBlock(juce::dsp::AudioBlock<float> &block)
    {
        mat1Param.update();
        mat2Param.update();
        mat3Param.update();
        mat4Param.update();
        mat5Param.update();
        mat6Param.update();
        mat7Param.update();
        mat8Param.update();
        mat9Param.update();

        for (int i = 0; i < block.getNumSamples(); i++)
        {
            float matrix1 = mat1Param.getNextValue();
            float matrix2 = mat2Param.getNextValue();
            float matrix3 = mat3Param.getNextValue();
            float matrix4 = mat4Param.getNextValue();
            // float matrix5 = mat5Param.getNextValue();
            // float matrix6 = mat6Param.getNextValue();
            // float matrix7 = mat7Param.getNextValue();
            // float matrix8 = mat8Param.getNextValue();
            // float matrix9 = mat9Param.getNextValue();

            double p1 = matrix1 * 10.0 + tiny;
            double p2 = matrix2 * 20.0 + tiny;
            // double p3 = matrix3;
            double p4 = matrix4 + tiny;

            for (int channel = 0; channel < block.getNumChannels(); channel++)
            {

                double x = block.getSample(channel, i);

                double sinWS = cursedSinWS(x, p2);
                double shredded = shredShape(x, p4);

                double res = tanhWaveShaper(x - sinWS + shredded, p1);

                block.setSample(channel, i, (float)res);
            }
        }
    }

private:
    double tiny = 0.001f;

    SmoothParam mat1Param;
    SmoothParam mat2Param;
    SmoothParam mat3Param;
    SmoothParam mat4Param;
    SmoothParam mat5Param;
    SmoothParam mat6Param;
    SmoothParam mat7Param;
    SmoothParam mat8Param;
    SmoothParam mat9Param;
};