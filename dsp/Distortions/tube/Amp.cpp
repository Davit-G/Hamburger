#include "Amp.h"

void Amp::prepare(dsp::ProcessSpec &spec)
{
    tubeTone.prepare(spec);

    inputHighPass.reset();
    *inputHighPass.state = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass(spec.sampleRate, 20.0f);

    inputHighPass.prepare(spec);    

    for (int i = 0; i < 4; i++)
    {
        triodes[i].prepare(spec);
    }

    sampleRate = spec.sampleRate;

    calculateCoefficients();
}

void Amp::calculateCoefficients()
{
    float driv = drive.getRaw() * 0.02f;

    auto &triode1 = triodes[0];
    auto &triode2 = triodes[1];
    auto &triode3 = triodes[2];
    auto &triode4 = triodes[3];

    // the divisions in this code would all be calculated at compile time
    // so not a big deal, just mentioning it tho
    triode1.lowFrequencyShelf_Hz = JUCE_LIVE_CONSTANT(10.0f);
    triode1.lowFrequencyShelfGain_dB = JUCE_LIVE_CONSTANT(-10.0f);
    triode1.millerHF_Hz = JUCE_LIVE_CONSTANT(20000.0f);
    triode1.dcBlockingLF_Hz = JUCE_LIVE_CONSTANT(15.0f);
    triode1.outputGain = pow(10.0f, JUCE_LIVE_CONSTANT(-1.0f) / 20.0f);
    triode1.dcShiftCoefficient = JUCE_LIVE_CONSTANT(1.0f);
    triode1.inputGain = driv + 0.3f;

    triode2.lowFrequencyShelf_Hz = JUCE_LIVE_CONSTANT(10.0f);
    triode2.lowFrequencyShelfGain_dB = JUCE_LIVE_CONSTANT(-10.0f);
    triode2.inputGain = driv + 0.8f;
    triode2.millerHF_Hz = 7000.0f + 6000.0f * tubeTone.getRaw();
    triode2.dcBlockingLF_Hz = JUCE_LIVE_CONSTANT(22.0f);
    triode2.outputGain = pow(10.0f, JUCE_LIVE_CONSTANT(+2.0f) / 20.0f);
    triode2.dcShiftCoefficient = JUCE_LIVE_CONSTANT(2.10f);

    triode3.lowFrequencyShelf_Hz = JUCE_LIVE_CONSTANT(10.0f);
    triode3.lowFrequencyShelfGain_dB = JUCE_LIVE_CONSTANT(-10.0f);
    triode3.millerHF_Hz = 9000.0f + 6000.0f * tubeTone.getRaw();
    // triode3.millerHF_Hz = 20000.0;
    triode3.inputGain = driv + 0.8f;
    triode3.dcBlockingLF_Hz = JUCE_LIVE_CONSTANT(36.0f);
    triode3.outputGain = pow(10.0f, JUCE_LIVE_CONSTANT(+4.0f) / 20.0f);
    triode3.dcShiftCoefficient = JUCE_LIVE_CONSTANT(0.50f);

    triode4.lowFrequencyShelf_Hz = JUCE_LIVE_CONSTANT(10.0f);
    triode4.lowFrequencyShelfGain_dB = JUCE_LIVE_CONSTANT(-10.0f);
    // trioesL[3].millerHF_Hz = 6400.0;
    triode4.millerHF_Hz = 8000.0f + 5000.0f * tubeTone.getRaw();
    triode4.dcBlockingLF_Hz = JUCE_LIVE_CONSTANT(38.0f);
    triode4.outputGain = pow(10.0f, JUCE_LIVE_CONSTANT(-12.0f) / 20.0f);
    triode4.dcShiftCoefficient = JUCE_LIVE_CONSTANT(0.52f);

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