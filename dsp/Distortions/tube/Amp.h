#include <JuceHeader.h>
#include "Valve.h"

#include "../../SmoothParam.h"

class Amp
{
public:
    Amp(juce::AudioProcessorValueTreeState &treeState) : tone(treeState, "tubeTone") {  
		calculateCoefficients();
    }

    ~Amp() {}

    void calculateCoefficients() {
        triodesL[0].lowFrequencyShelf_Hz = 10.0;
		triodesL[0].lowFrequencyShelfGain_dB = -10.0;
		triodesL[0].millerHF_Hz = 20000.0;
		triodesL[0].dcBlockingLF_Hz = 5.0;
		triodesL[0].outputGain = pow(10.0, -4.0 / 20.0);
		triodesL[0].dcShiftCoefficient = 1.0;

		triodesL[1].lowFrequencyShelf_Hz = 10.0;
		triodesL[1].lowFrequencyShelfGain_dB = -10.0;
		triodesL[1].millerHF_Hz = 20000.0;
		triodesL[1].dcBlockingLF_Hz = 38.0;
		triodesL[1].outputGain = pow(10.0, +3.0 / 20.0);
		triodesL[1].dcShiftCoefficient = 0.20;

		triodesL[2].lowFrequencyShelf_Hz = 10.0;
		triodesL[2].lowFrequencyShelfGain_dB = -10.0;
		triodesL[2].millerHF_Hz = 7000.0 + 6000.0 * tone.getRaw();
		// triodesL[2].millerHF_Hz = 20000.0;
		triodesL[2].dcBlockingLF_Hz = 10.0;
		triodesL[2].outputGain = pow(10.0, +4.0 / 20.0);
		triodesL[2].dcShiftCoefficient = 0.50;

		triodesL[3].lowFrequencyShelf_Hz = 10.0;
		triodesL[3].lowFrequencyShelfGain_dB = -10.0;
		// triodesL[3].millerHF_Hz = 6400.0;
		triodesL[3].millerHF_Hz = 20000.0;
		triodesL[3].dcBlockingLF_Hz = 33.0;
		triodesL[3].outputGain = pow(10.0, -15.0 / 20.0);
		triodesL[3].dcShiftCoefficient = 0.52;
        
        for (int i = 0; i < 4; i++)
        {
            triodesR[i].lowFrequencyShelf_Hz = triodesL[i].lowFrequencyShelf_Hz;
            triodesR[i].lowFrequencyShelfGain_dB = triodesL[i].lowFrequencyShelfGain_dB;
            triodesR[i].millerHF_Hz = triodesL[i].millerHF_Hz;
            triodesR[i].dcBlockingLF_Hz = triodesL[i].dcBlockingLF_Hz;
            triodesR[i].outputGain = triodesL[i].outputGain;
            triodesR[i].dcShiftCoefficient = triodesL[i].dcShiftCoefficient;

            triodesL[i].calculateCoefficients();
            triodesR[i].calculateCoefficients();
        }
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock)
    {
        tone.prepareToPlay(sampleRate, samplesPerBlock);

        dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = samplesPerBlock;
        spec.numChannels = 2; // eh

        inputHighPass.reset();
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
        tone.update();

        calculateCoefficients();

        // step 1: remove DC with highpass filter
        inputHighPass.process(dsp::ProcessContextReplacing<float>(block));

        // step 2: apply gain
        block.multiplyBy(inputGain);

        // left channel
        for (int j = 0; j < block.getNumSamples(); j++)
        {
            auto sampleL = block.getSample(0, j);
            
            sampleL = triodesL[0].processAudioSample(sampleL); // step 3: triode 1
            sampleL *= driveGain; // step 4: drive the signal

            // step 5: triode 2 and 3 and etc, go nuts
            sampleL = triodesL[1].processAudioSample(sampleL);
            sampleL = triodesL[2].processAudioSample(sampleL);
            sampleL = triodesL[3].processAudioSample(sampleL);
            
            sampleL *= tubeCompress; // step 6: class B

            // we shall ignore class B for now
            block.setSample(0, j, sampleL);
        }

        // do same for right channel
        for (int j = 0; j < block.getNumSamples(); j++)
        {
            auto sampleR = block.getSample(1, j);
            
            sampleR = triodesR[0].processAudioSample(sampleR); // step 3: triode 1
            sampleR *= driveGain; // step 4: drive the signal

            // step 5: triode 2 and 3 and etc, go nuts
            sampleR = triodesR[1].processAudioSample(sampleR);
            sampleR = triodesR[2].processAudioSample(sampleR);
            sampleR = triodesR[3].processAudioSample(sampleR);
            // step 6: class B
            sampleR *= tubeCompress;

            // we shall ignore class B for now

            block.setSample(1, j, sampleR);
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

    SmoothParam tone;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Amp)
};