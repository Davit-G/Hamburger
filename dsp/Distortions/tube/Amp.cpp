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
        triodes[i].prepare(spec);
    }
}

void Amp::calculateCoefficients()
{
    float driv = drive.getRaw() * 0.02f;

    // the divisions in this code would all be calculated at compile time
    // so not a big deal, just mentioning it tho
    triodes[0].lowFrequencyShelf_Hz = 10.0f;
    triodes[0].lowFrequencyShelfGain_dB = -10.0f;
    triodes[0].millerHF_Hz = 20000.0f;
    triodes[0].dcBlockingLF_Hz = 15.0f;
    triodes[0].outputGain = pow(10.0f, -4.0f / 20.0f);
    triodes[0].dcShiftCoefficient = 1.0f;
    triodes[0].inputGain = driv + 0.3f;
    triodes[1].lowFrequencyShelf_Hz = 10.0f;
    triodes[1].lowFrequencyShelfGain_dB = 0.0f;
    triodes[1].inputGain = driv + 0.8f;
    triodes[1].millerHF_Hz = 7000.0f + 6000.0f * tubeTone.getRaw();
    triodes[1].dcBlockingLF_Hz = 38.0f;
    triodes[1].outputGain = pow(10.0f, +6.0f / 20.0f);
    triodes[1].dcShiftCoefficient = 2.10f;
    triodes[2].lowFrequencyShelf_Hz = 10.0f;
    triodes[2].lowFrequencyShelfGain_dB = -10.0f;
    triodes[2].millerHF_Hz = 9000.0f + 6000.0f * tubeTone.getRaw();
    // trioesL[2].millerHF_Hz = 20000.0;
    triodes[2].inputGain = driv + 0.8f;
    triodes[2].dcBlockingLF_Hz = 36.0f;
    triodes[2].outputGain = pow(10.0f, +4.0f / 20.0f);
    triodes[2].dcShiftCoefficient = 0.50f;
    triodes[3].lowFrequencyShelf_Hz = 10.0f;
    triodes[3].lowFrequencyShelfGain_dB = -10.0f;
    // trioesL[3].millerHF_Hz = 6400.0;
    triodes[3].millerHF_Hz = 8000.0f + 5000.0f * tubeTone.getRaw();
    triodes[3].dcBlockingLF_Hz = 38.0f;
    triodes[3].outputGain = pow(10.0f, -12.0f / 20.0f);
    triodes[3].dcShiftCoefficient = 0.52f;

    for (int i = 0; i < 4; i++)
    {
        triodes[i].calculateCoefficients();
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
    TRACE_EVENT_BEGIN("dsp", "tubes");

    triodes[0].processBlock(block); // step 3: triode 1
    block.multiplyBy(driveGain);    // step 4: drive the signal

    // step 5: triode 2 and 3 and etc, go nuts
    triodes[1].processBlock(block);
    triodes[2].processBlock(block);
    triodes[3].processBlock(block);

    block.multiplyBy(tubeCompress);
    TRACE_EVENT_END("dsp");

    // step 7: tone stack?
    // step 8: output gain
    block.multiplyBy(outputGain);
}