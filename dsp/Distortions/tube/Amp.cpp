#include "Amp.h"

void Amp::prepare(juce::dsp::ProcessSpec &spec)
{
    tubeTone.prepare(spec);
    bias.prepare(spec);
    drive.prepare(spec);


    inputHighPass.reset();
    *inputHighPass.state = juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderHighPass(spec.sampleRate, 20.0f);

    inputHighPass.prepare(spec);

    for (int i = 0; i < 4; i++)
    {
        triodes[i].prepare(spec);
    }

    sampleRate = spec.sampleRate;


    calcCoefficients();
}

void Amp::calcCoefficientsPerSample() {
    float nextDrive = drive.getNextValue();
    float driv = nextDrive * 0.02f;

    auto tubeToneValue = tubeTone.getNextValue();
    auto skewedTone = tubeToneValue*tubeToneValue*tubeToneValue;

    auto &triode1 = triodes[0];
    auto &triode2 = triodes[1];
    auto &triode3 = triodes[2];
    auto &triode4 = triodes[3];

    for (int i = 0; i < 4; i++)
    {
        // triodes[i].blend = blend * 4;
        triodes[i].waveshaperSaturation = 0.01f + powf(nextDrive * 0.02f, 0.8f) * 1.6f;
    }

    triode1.millerHF_Hz = fmin(6000.0f + 14000.0f * skewedTone, sampleRate * 0.49f);
    triode1.inputGain = driv + 0.3f;
    triode2.millerHF_Hz = fmin(7000.0f + 14000.0f * skewedTone, sampleRate * 0.49f);
    triode3.millerHF_Hz = fmin(9000.0f + 14000.0f * skewedTone, sampleRate * 0.49f);

    triode4.outputGain = juce::Decibels::decibelsToGain(-nextDrive * 0.01f * 17.f - 1.5f);
}

void Amp::calcCoefficients()
{
    calcCoefficientsPerSample();

    auto &triode1 = triodes[0];
    auto &triode2 = triodes[1];
    auto &triode3 = triodes[2];
    auto &triode4 = triodes[3];


    for (int i = 0; i < 4; i++)
    {
        // triodes[i].clipPointPositive = 4.0f;
        // triodes[i].clipPointNegative = (-1.5f);
        // triodes[i].gridConductionThreshold = (1.9f);
    }

    triode1.lowFrequencyShelf_Hz = (10.0f);
    triode1.lowFrequencyShelfGain_dB = (-10.0f);
    // triode1.millerHF_Hz = (20000.0f);
    triode1.dcBlockingLF_Hz = (12.0f);
    triode1.outputGain = pow(10.0f, (+5.0f) / 20.0f);
    triode1.dcShiftCoefficient = (1.0f);

    triode2.lowFrequencyShelf_Hz = (10.0f);
    triode2.lowFrequencyShelfGain_dB = (-10.0f);
    // triode2.inputGain = driv + 0.8f;
    triode2.dcBlockingLF_Hz = (25.0f);
    triode2.outputGain = pow(10.0f, (+2.0f) / 20.0f);
    triode2.dcShiftCoefficient = (2.10f);

    triode3.lowFrequencyShelf_Hz = (10.0f);
    triode3.lowFrequencyShelfGain_dB = (-10.0f);
    // triode3.millerHF_Hz = 22000.0;
    // triode3.inputGain = driv + 0.8f;
    triode3.dcBlockingLF_Hz = (10.0f);
    triode3.outputGain = pow(10.0f, (+4.0f) / 20.0f);
    triode3.dcShiftCoefficient = (0.50f);

    triode4.lowFrequencyShelf_Hz = (10.0f);
    triode4.lowFrequencyShelfGain_dB = (-6.0f);
    // trioesL[3].millerHF_Hz = 6400.0;
    triode4.millerHF_Hz = 20000.0f;
    triode4.dcBlockingLF_Hz = (8.0f);
    triode4.dcShiftCoefficient = (0.52f);

    for (int i = 0; i < 4; i++)
    {
        triodes[i].calculateCoefficients();
    }
}

void Amp::calcCoefficients2()
{
    float driv = drive.getRaw() * 0.02f;

    auto &triode1 = triodes[0];
    auto &triode2 = triodes[1];
    auto &triode3 = triodes[2];
    auto &triode4 = triodes[3];

    auto lowFreqShelfHz = (9.0f);
    auto loShelfGain = (-6.0f);

    auto biasAmt = bias.getRaw() * 2.0f;

    auto inGain = 1.f + driv * 0.4f;
    auto outGain = 1.1f;

    auto blend = drive.getRaw() * 0.005f;

    for (int i = 0; i < 4; i++)
    {
        // triodes[i].blend = blend;
        // triodes[i].blend = blend * 4;
        triodes[i].waveshaperSaturation = 0.01f + drive.getRaw() * 0.03;
        triodes[i].clipPointPositive = 4.0f;
        triodes[i].clipPointNegative = (-1.5f);
        triodes[i].gridConductionThreshold = (1.5f);
        triodes[i].dcShiftCoefficient = (1.2f);
        triodes[i].lowFrequencyShelf_Hz = lowFreqShelfHz;
        triodes[i].lowFrequencyShelfGain_dB = loShelfGain;
        triodes[i].dcBlockingLF_Hz = (5.0f);
    }

    auto skewedTone = powf(tubeTone.getRaw(), 3.0f);

    triode1.millerHF_Hz = fmin(16000.0f + 8000.0f * skewedTone, 21000.0f);
    triode1.outputGain = outGain;
    triode1.dcShiftAdditional = biasAmt;
    triode1.dcShiftCoefficient = 1.0f + biasAmt;
    triode1.inputGain = 1.0f + driv * 0.4f;

    triode2.inputGain = inGain;
    triode2.millerHF_Hz = fmin(6000.0f + 7900.0f * skewedTone, 21000.0f);
    triode2.outputGain = outGain;
    triode2.dcShiftAdditional = biasAmt;
    triode2.dcShiftCoefficient = 1.2f;

    triode3.millerHF_Hz = fmin(9000.0f + 7900.0f * skewedTone, 21000.0f);
    // triode3.millerHF_Hz = 20000.0;
    triode3.inputGain = inGain;
    triode3.outputGain = outGain;
    // triode3.dcShiftAdditional = biasAmt;
    triode3.dcShiftCoefficient = 1.f;

    // trioesL[3].millerHF_Hz = 6400.0;
    triode4.millerHF_Hz = 21000.0f;
    triode4.outputGain = pow(10.0f, (-drive.getRaw() * 0.01f * 6.f) / 20.0f);
    triode4.dcShiftCoefficient = 1.2f;

    for (int i = 0; i < 4; i++)
    {
        triodes[i].calculateCoefficients();
    }
}

void Amp::processBlock(juce::dsp::AudioBlock<float> &block)
{
    // TRACE_EVENT_BEGIN("dsp", "tube coefficients");
    tubeTone.update();
    drive.update();
    bias.update();

    calcCoefficients();
    // TRACE_EVENT_END("dsp");

    // step 1: remove DC with highpass filter
    // inputHighPass.process(dsp::ProcessContextReplacing<float>(block));

    // step 2: apply gain
    block.multiplyBy(inputGain);

    // TRACE_EVENT_BEGIN("dsp", "tubes");

    for (int i = 0; i < block.getNumSamples(); ++i)
    {   
        calcCoefficientsPerSample();

        float biasAmt = bias.getNextValue();
        float biasToAdd = biasAmt * biasAmt * -1.6f;

        for (int ch = 0; ch < block.getNumChannels(); ++ch)
        {
            auto interm = block.getSample(ch, i) + biasToAdd;

            interm = triodes[0].processAudioSample(interm, ch);

            interm *= driveGain;
            interm = triodes[2].processAudioSample(interm, ch);
            interm = -triodes[3].processAudioSample(interm, ch);

            block.setSample(ch, i, interm);
        }
    }

    // triodes[0].processBlock(block); // step 3: triode 1
    // block.multiplyBy(driveGain);    // step 4: drive the signal

    // step 5: triode 2 and 3 and etc, go nuts
    // // triodes[1].processBlock(block);
    // triodes[2].processBlock(block);
    // triodes[3].processBlock(block);

    block.multiplyBy(tubeCompress);
    // TRACE_EVENT_END("dsp");

    // step 7: tone stack?
    // step 8: output gain
    // block.multiplyBy(outputGain);
}