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

    auto lowFreqShelfHz = JUCE_LIVE_CONSTANT(10.0f);
    auto loShelfGain = JUCE_LIVE_CONSTANT(-10.0f);

    auto biasAmt = bias.getRaw() * 60.0f;

    auto inGain = 0.7f + driv;
    auto outGain = 1.1f;

    auto blend = drive.getRaw() * 0.005f;
    triode1.blend = blend;
    triode2.blend = blend;
    triode3.blend = blend;
    triode4.blend = blend;

    // the divisions in this code would all be calculated at compile time
    // so not a big deal, just mentioning it tho
    triode1.lowFrequencyShelf_Hz = lowFreqShelfHz;
    triode1.lowFrequencyShelfGain_dB = loShelfGain;
    triode1.millerHF_Hz = JUCE_LIVE_CONSTANT(20000.0f);
    triode1.dcBlockingLF_Hz = JUCE_LIVE_CONSTANT(12.0f);
    triode1.outputGain = outGain;
    triode1.dcShiftCoefficient = biasAmt;
    triode1.inputGain = driv + 0.3f;

    triode2.lowFrequencyShelf_Hz = lowFreqShelfHz;
    triode2.lowFrequencyShelfGain_dB = loShelfGain;
    triode2.inputGain = inGain;
    triode2.millerHF_Hz = 7000.0f + 6000.0f * tubeTone.getRaw();
    triode2.dcBlockingLF_Hz = JUCE_LIVE_CONSTANT(16.0f);
    triode2.outputGain = outGain;
    triode2.dcShiftCoefficient = biasAmt;

    triode3.lowFrequencyShelf_Hz = lowFreqShelfHz;
    triode3.lowFrequencyShelfGain_dB = loShelfGain;
    triode3.millerHF_Hz = 9000.0f + 6000.0f * tubeTone.getRaw();
    // triode3.millerHF_Hz = 20000.0;
    triode3.inputGain = inGain;
    triode3.dcBlockingLF_Hz = JUCE_LIVE_CONSTANT(10.0f);
    triode3.outputGain = outGain;
    triode3.dcShiftCoefficient = biasAmt;

    triode4.lowFrequencyShelf_Hz = lowFreqShelfHz;
    triode4.lowFrequencyShelfGain_dB = loShelfGain;
    // trioesL[3].millerHF_Hz = 6400.0;
    triode4.millerHF_Hz = 8000.0f + 5000.0f * tubeTone.getRaw();
    triode4.dcBlockingLF_Hz = JUCE_LIVE_CONSTANT(6.0f);
    triode4.outputGain = pow(10.0f, JUCE_LIVE_CONSTANT(-12.0f) / 20.0f);
    triode4.dcShiftCoefficient = biasAmt * 0.3f;

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