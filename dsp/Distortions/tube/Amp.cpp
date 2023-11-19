#include "Amp.h"
#include <JuceHeader.h>

void Amp::prepare(dsp::ProcessSpec &spec)
{
    tubeTone.prepare(spec);

    inputHighPass.reset();
    juce::dsp::IIR::Coefficients<float>::Ptr inputHighPassCoefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(spec.sampleRate, 20.0f);
    inputHighPass.state = *inputHighPassCoefficients;

    inputHighPass.prepare(spec);

    for (int i = 0; i < 4; i++)
    {
        triodesL[i].prepare(spec);
        triodesR[i].prepare(spec);
    }
}

void Amp::calculateCoefficients()
{
    float driv = drive.getRaw() * 0.02f;

    // the divisions in this code would all be calculated at compile time
    // so not a big deal, just mentioning it tho
    triodesL[0].lowFrequencyShelf_Hz = 10.0f;
    triodesL[0].lowFrequencyShelfGain_dB = -10.0f;
    triodesL[0].millerHF_Hz = 20000.0f;
    triodesL[0].dcBlockingLF_Hz = 15.0f;
    triodesL[0].outputGain = pow(10.0f, -4.0f / 20.0f);
    triodesL[0].dcShiftCoefficient = 1.0f;
    triodesL[0].inputGain = driv + 0.3f;

    triodesL[1].lowFrequencyShelf_Hz = 10.0f;
    triodesL[1].lowFrequencyShelfGain_dB = 0.0f;
    triodesL[1].inputGain = driv + 0.8f;
    triodesL[1].millerHF_Hz = 7000.0f + 6000.0f * tubeTone.getRaw();
    triodesL[1].dcBlockingLF_Hz = 38.0f;
    triodesL[1].outputGain = pow(10.0f, +6.0f / 20.0f);
    triodesL[1].dcShiftCoefficient = 2.10f;

    triodesL[2].lowFrequencyShelf_Hz = 10.0f;
    triodesL[2].lowFrequencyShelfGain_dB = -10.0f;
    triodesL[2].millerHF_Hz = 9000.0f + 6000.0f * tubeTone.getRaw();
    // triodesL[2].millerHF_Hz = 20000.0;
    triodesL[2].inputGain = driv + 0.8f;
    triodesL[2].dcBlockingLF_Hz = 36.0f;
    triodesL[2].outputGain = pow(10.0f, +4.0f / 20.0f);
    triodesL[2].dcShiftCoefficient = 0.50f;

    triodesL[3].lowFrequencyShelf_Hz = 10.0f;
    triodesL[3].lowFrequencyShelfGain_dB = -10.0f;
    // triodesL[3].millerHF_Hz = 6400.0;
    triodesL[3].millerHF_Hz = 8000.0f + 5000.0f * tubeTone.getRaw();
    triodesL[3].dcBlockingLF_Hz = 38.0f;
    triodesL[3].outputGain = pow(10.0f, -12.0f / 20.0f);
    triodesL[3].dcShiftCoefficient = 0.52f;

    for (int i = 0; i < 4; i++)
    {
        triodesR[i].lowFrequencyShelf_Hz = triodesL[i].lowFrequencyShelf_Hz;
        triodesR[i].lowFrequencyShelfGain_dB = triodesL[i].lowFrequencyShelfGain_dB;
        triodesR[i].millerHF_Hz = triodesL[i].millerHF_Hz;
        triodesR[i].dcBlockingLF_Hz = triodesL[i].dcBlockingLF_Hz;
        triodesR[i].outputGain = triodesL[i].outputGain;
        triodesR[i].dcShiftCoefficient = triodesL[i].dcShiftCoefficient;
        triodesR[i].inputGain = triodesL[i].inputGain;

        triodesL[i].calculateCoefficients();
        triodesR[i].calculateCoefficients();
    }
}

void Amp::processBlock(dsp::AudioBlock<float> &block)
{
    TRACE_EVENT_BEGIN("dsp", "tube coefficients");
    tubeTone.update();

    calculateCoefficients();
    TRACE_EVENT_END("dsp");

    // step 1: remove DC with highpass filter
    TRACE_EVENT_BEGIN("dsp", "highpass");
    inputHighPass.process(dsp::ProcessContextReplacing<float>(block));
    TRACE_EVENT_END("dsp");

    // step 2: apply gain
    block.multiplyBy(inputGain);

    // left channel
    TRACE_EVENT_BEGIN("dsp", "left channel tube");

    const size_t numSamples = block.getNumSamples();
    for (size_t j = 0; j < numSamples; j++)
    {
        auto sampleL = block.getSample(0, j);

        sampleL = triodesL[0].processAudioSample(sampleL); // step 3: triode 1
        sampleL *= driveGain;                              // step 4: drive the signal

        // step 5: triode 2 and 3 and etc, go nuts
        sampleL = triodesL[1].processAudioSample(sampleL);
        sampleL = triodesL[2].processAudioSample(sampleL);
        sampleL = triodesL[3].processAudioSample(sampleL);

        sampleL *= tubeCompress; // step 6: class B

        // we shall ignore class B for now
        block.setSample(0, j, sampleL);
    }
    TRACE_EVENT_END("dsp");

    TRACE_EVENT_BEGIN("dsp", "right channel tube");
    // do same for right channel
    for (size_t j = 0; j < numSamples; j++)
    {
        auto sampleR = block.getSample(1, j);

        sampleR = triodesR[0].processAudioSample(sampleR); // step 3: triode 1
        sampleR *= driveGain;                              // step 4: drive the signal

        // step 5: triode 2 and 3 and etc, go nuts
        sampleR = triodesR[1].processAudioSample(sampleR);
        sampleR = triodesR[2].processAudioSample(sampleR);
        sampleR = triodesR[3].processAudioSample(sampleR);
        // step 6: class B
        sampleR *= tubeCompress;

        // we shall ignore class B for now

        block.setSample(1, j, sampleR);
    }
    TRACE_EVENT_END("dsp");

    // step 7: tone stack?
    // step 8: output gain
    block.multiplyBy(outputGain);
}