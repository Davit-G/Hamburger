#pragma once

 

struct HilbertTransformOutput
{
    float x;
    float y;
};

class OneSampleDelay
{
public:
    OneSampleDelay() : prevSample_(0) {}

    float processSample(float x)
    {
        float y = prevSample_;
        prevSample_ = x;
        return y;
    }

private:
    float prevSample_;
};

class CustomBiquadFilter
{
public:
    CustomBiquadFilter(float a0, float a1, float a2, float b1, float b2)
        : a0_(a0), a1_(a1), a2_(a2), b1_(b1), b2_(b2), x1_(0), x2_(0), y1_(0), y2_(0) {}

    float processSample(float x)
    {
        float y = a0_ * x + a1_ * x1_ + a2_ * x2_ - b1_ * y1_ - b2_ * y2_;

        // Update delay lines
        x2_ = x1_;
        x1_ = x;
        y2_ = y1_;
        y1_ = y;

        return y;
    }

private:
    float a0_, a1_, a2_, b1_, b2_; // Filter coefficients
    float x1_, x2_, y1_, y2_;      // Delay lines for x and y
};

class HilbertBiquadTransform
{
public:
    HilbertBiquadTransform()
    {
    }
    ~HilbertBiquadTransform() {}

    HilbertTransformOutput processSample(float x)
    {
        // first phase all-pass cascade

        float xcopy = x;

        float firstPhaseResult = bql1.processSample(x);
        firstPhaseResult = bql2.processSample(firstPhaseResult);
        firstPhaseResult = bql3.processSample(firstPhaseResult);
        firstPhaseResult = bql4.processSample(firstPhaseResult);

        float secondPhaseResult = delay.processSample(xcopy);
        secondPhaseResult = bqr1.processSample(secondPhaseResult);
        secondPhaseResult = bqr2.processSample(secondPhaseResult);
        secondPhaseResult = bqr3.processSample(secondPhaseResult);
        secondPhaseResult = bqr4.processSample(secondPhaseResult);

        HilbertTransformOutput output;
        output.x = firstPhaseResult;
        output.y = secondPhaseResult;

        return output;
    }

private:
    CustomBiquadFilter bql1{0.161758, 0., -1, 0, -0.1617158};
    CustomBiquadFilter bql2{0.733029, 0, -1, 0, -0.733029};
    CustomBiquadFilter bql3{0.94535, 0., -1, 0, -0.94535};
    CustomBiquadFilter bql4{0.990598, 0., -1, 0, -0.990598};

    CustomBiquadFilter bqr1{0.479401, 0., -1, 0, -0.479401};
    CustomBiquadFilter bqr2{0.876218, 0., -1, 0, -0.876218};
    CustomBiquadFilter bqr3{0.976599, 0., -1, 0, -0.976599};
    CustomBiquadFilter bqr4{0.9975, 0., -1, 0, -0.9975};

    OneSampleDelay delay;
};

class FrequencyShifterAlt
{
public:
    FrequencyShifterAlt(){

    };

    ~FrequencyShifterAlt(){};

    void prepareToPlay(double sampleRate, int samplesPerBlock)
    {
        dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = samplesPerBlock;
        spec.numChannels = 2;

        m_sampleRate = sampleRate;
    };

    void processBlock(dsp::AudioBlock<float> &block)
    {
        phaseIncrement = freqShiftHz * (1 / m_sampleRate);

        for (int i = 0; i < block.getNumSamples(); i++)
        {
            HilbertTransformOutput hilbertLeft = hilberFreqShifterL.processSample(block.getSample(0, i));
            HilbertTransformOutput hilbertRight = hilberFreqShifterR.processSample(block.getSample(1, i));
            float QL = hilbertLeft.x * 0.5;
            float IL = hilbertLeft.y * 0.5;

            float QR = hilbertRight.x * 0.5;
            float IR = hilbertRight.y * 0.5;

            phase = fmodf(phase + phaseIncrement, 1.f);
            float theta = 2 * juce::float_Pi * phase; // this seems to be the variable responsible for the oscillator shift

            block.setSample(0, i, 2 * (QL * cos(theta) - IL * sin(theta)));
            block.setSample(1, i, 2 * (QR * cos(theta) - IR * sin(theta)));
        };
    };

    void setFrequencyShift(float newFrequencyShift)
    {
        freqShiftHz = newFrequencyShift;
    };

private:
    //==============================================================================

    float m_dryWetMix{1.f};

    HilbertBiquadTransform hilberFreqShifterL;
    HilbertBiquadTransform hilberFreqShifterR;

    float freqShiftHz = 0.0f;
    double m_sampleRate;

    float phaseIncrement = 0.0f;
    float phase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrequencyShifterAlt)
};