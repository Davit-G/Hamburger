

#include <JuceHeader.h>
#include "Valve.h"

class Amp
{
public:
    Amp() {
		triodesL[0].lowFrequencyShelf_Hz = 10.0;
		triodesL[0].lowFrequencyShelfGain_dB = -10.0;
		triodesL[0].millerHF_Hz = 20000.0;
		triodesL[0].dcBlockingLF_Hz = 8.0;
		triodesL[0].outputGain = pow(10.0, -3.0 / 20.0);
		triodesL[0].dcShiftCoefficient = 1.0;

		triodesL[1].lowFrequencyShelf_Hz = 10.0;
		triodesL[1].lowFrequencyShelfGain_dB = -10.0;
		triodesL[1].millerHF_Hz = 9000.0;
		triodesL[1].dcBlockingLF_Hz = 32.0;
		triodesL[1].outputGain = pow(10.0, +5.0 / 20.0);
		triodesL[1].dcShiftCoefficient = 0.20;

		triodesL[2].lowFrequencyShelf_Hz = 10.0;
		triodesL[2].lowFrequencyShelfGain_dB = -10.0;
		triodesL[2].millerHF_Hz = 7000.0;
		triodesL[2].dcBlockingLF_Hz = 40.0;
		triodesL[2].outputGain = pow(10.0, +6.0 / 20.0);
		triodesL[2].dcShiftCoefficient = 0.50;

		triodesL[3].lowFrequencyShelf_Hz = 10.0;
		triodesL[3].lowFrequencyShelfGain_dB = -10.0;
		triodesL[3].millerHF_Hz = 6400.0;
		triodesL[3].dcBlockingLF_Hz = 43.0;
		triodesL[3].outputGain = pow(10.0, -20.0 / 20.0);
		triodesL[3].dcShiftCoefficient = 0.52;

        triodesR[0].lowFrequencyShelf_Hz = 10.0;
		triodesR[0].lowFrequencyShelfGain_dB = -10.0;
		triodesR[0].millerHF_Hz = 20000.0;
		triodesR[0].dcBlockingLF_Hz = 8.0;
		triodesR[0].outputGain = pow(10.0, -3.0 / 20.0);
		triodesR[0].dcShiftCoefficient = 1.0;

		triodesR[1].lowFrequencyShelf_Hz = 10.0;
		triodesR[1].lowFrequencyShelfGain_dB = -10.0;
		triodesR[1].millerHF_Hz = 9000.0;
		triodesR[1].dcBlockingLF_Hz = 32.0;
		triodesR[1].outputGain = pow(10.0, +5.0 / 20.0);
		triodesR[1].dcShiftCoefficient = 0.20;

		triodesR[2].lowFrequencyShelf_Hz = 10.0;
		triodesR[2].lowFrequencyShelfGain_dB = -10.0;
		triodesR[2].millerHF_Hz = 7000.0;
		triodesR[2].dcBlockingLF_Hz = 40.0;
		triodesR[2].outputGain = pow(10.0, +6.0 / 20.0);
		triodesR[2].dcShiftCoefficient = 0.50;

		triodesR[3].lowFrequencyShelf_Hz = 10.0;
		triodesR[3].lowFrequencyShelfGain_dB = -10.0;
		triodesR[3].millerHF_Hz = 6400.0;
		triodesR[3].dcBlockingLF_Hz = 43.0;
		triodesR[3].outputGain = pow(10.0, -20.0 / 20.0);
		triodesR[3].dcShiftCoefficient = 0.52;
    }

    ~Amp() {}

    void prepareToPlay(double sampleRate, int samplesPerBlock)
    {
        dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = samplesPerBlock;
        spec.numChannels = 2; // eh

        inputHighPass.reset();
        // set it to 20hz for now, but I want to make this a parameter
        juce::dsp::IIR::Coefficients<float>::Ptr inputHighPassCoefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 20.0f);
        inputHighPass.state = *inputHighPassCoefficients;

        inputHighPass.prepare(spec);


        for (int i = 0; i < 4; i++)
        {
            triodesL[i].prepareToPlay(sampleRate, samplesPerBlock);
            triodesR[i].prepareToPlay(sampleRate, samplesPerBlock);
        }
    }

    void processBlock(dsp::AudioBlock<float> &block)
    {   
        // step 1: remove DC with highpass filter
        inputHighPass.process(dsp::ProcessContextReplacing<float>(block));

        // step 2: apply gain
        block.multiplyBy(inputGain);

        // left channel
        auto leftChannel = block.getChannelPointer(0);
        for (int j = 0; j < block.getNumSamples(); j++)
        {
            auto sample = leftChannel[j];
            
            // step 3: triode 1
            sample = triodesL[0].processAudioSample(sample);

            // step 4: drive the signal
            sample *= driveGain;

            // step 5: triode 2 and 3 and etc, go nuts
            sample = triodesL[1].processAudioSample(sample);
            sample = triodesL[2].processAudioSample(sample);
            sample = triodesL[3].processAudioSample(sample);

            // step 6: class B
            sample *= tubeCompress;

            // we shall ignore class B for now

            block.setSample(0, j, sample);
        }

        // do same for right channel
        auto rightChannel = block.getChannelPointer(1);
        for (int j = 0; j < block.getNumSamples(); j++)
        {
            auto sample = rightChannel[j];
            
            // step 3: triode 1
            triodesR[0].processAudioSample(sample);

            // step 4: drive the signal
            sample *= driveGain;

            // step 5: triode 2 and 3 and etc, go nuts
            sample = triodesR[1].processAudioSample(sample);
            sample = triodesR[2].processAudioSample(sample);
            sample = triodesR[3].processAudioSample(sample);
            // step 6: class B
            sample *= tubeCompress;

            // we shall ignore class B for now

            block.setSample(1, j, sample);
        }
        
        // step 7: tone stack?
        // step 8: output gain
        block.multiplyBy(outputGain);
    }

    float inputGain = 1.0f;
    float driveGain = 1.0f;
    float tubeCompress = 1.0;
	float outputGain = 1.0;

private:
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> inputHighPass;

    ClassAValve triodesL[4];
    ClassAValve triodesR[4];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Amp)
};