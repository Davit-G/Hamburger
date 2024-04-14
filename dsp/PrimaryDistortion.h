#pragma once

#include "Distortions/SoftClipper.h"
#include "Distortions/PattyFuzz.h"
#include "Distortions/Fuzz.h"
#include "Distortions/Cooked.h"
#include "Distortions/DiodeWaveshape.h"
#include "Distortions/PhaseDist.h"
#include "Distortions/Rubidium.h"
#include "Distortions/MatrixWaveshaper.h"
#include "./Noise/Jeff.h"
#include "Distortions/tube/Amp.h"

#include "juce_core/juce_core.h"
#include "juce_dsp/juce_dsp.h"
#include "juce_audio_processors/juce_audio_processors.h"

#include <melatonin_perfetto/melatonin_perfetto.h>

class PrimaryDistortion
{
public:
    PrimaryDistortion(juce::AudioProcessorValueTreeState &state)
    {
        distoType = dynamic_cast<juce::AudioParameterChoice *>(state.getParameter("primaryDistortionType"));
        jassert(distoType);

        distortionEnabled = dynamic_cast<juce::AudioParameterBool *>(state.getParameter("primaryDistortionEnabled"));
        jassert(distortionEnabled);

        softClipper = std::make_unique<SoftClip>(state);
        fold = std::make_unique<Cooked>(state);
        patty = std::make_unique<PattyFuzz>(state);
        jeff = std::make_unique<Jeff>(state);
        fuzz = std::make_unique<Fuzz>(state);
        tubeAmp = std::make_unique<Amp>(state);
        phaseDist = std::make_unique<PhaseDist>(state);
        diodeWaveshape = std::make_unique<DiodeWaveshape>(state);
        rubidium = std::make_unique<RubidiumDistortion>(state);
        matrix = std::make_unique<MatrixWaveshaper>(state);
    }

    ~PrimaryDistortion() {}

    void processBlock(dsp::AudioBlock<float> &block)
    {
        TRACE_DSP();
        int distoTypeIndex = distoType->getIndex();

        if (distortionEnabled->get() == false)
            return;

        auto context = dsp::ProcessContextReplacing<float>(block);

        switch (distoTypeIndex)
        {
        case 0:
        { // classic

            TRACE_EVENT("dsp", "classic");
            // patty->processBlock(block);
            fuzz->processBlock(block);

            fold->processBlock(block);
            diodeWaveshape->processBlock(block);
            softClipper->processBlock(block);

            juce::AudioBuffer<double> bufferDouble(block.getNumChannels(), block.getNumSamples());
            juce::dsp::AudioBlock<double> blockDouble(bufferDouble);
            
            
            for (int channel = 0; channel < block.getNumChannels(); channel++)
            {
                for (int sample = 0; sample < block.getNumSamples(); sample++)
                {
                    blockDouble.setSample(channel, sample, block.getSample(channel, sample));
                }
            }

            auto doubleContext = dsp::ProcessContextReplacing<double>(blockDouble);

            iirFilter.process(doubleContext);
            
            for (int channel = 0; channel < block.getNumChannels(); channel++)
            {
                for (int sample = 0; sample < block.getNumSamples(); sample++)
                {
                    block.setSample(channel, sample, blockDouble.getSample(channel, sample));
                }
            }
            
            
            break;
        }
        case 1:
        { // tube
            TRACE_EVENT("dsp", "tube");
            jeff->processBlock(block);
            tubeAmp->processBlock(block);
            break;
        }
        case 2:
        { // phase distortion
            TRACE_EVENT("dsp", "phase");
            phaseDist->processBlock(block);
            break;
        }
        case 3:
        {// rubidium distortion
            TRACE_EVENT("dsp", "rubidium");
            rubidium->processBlock(block);
            break;
        }
        case 4:
        {// waveshaping matrix distortion
            TRACE_EVENT("dsp", "matrix");
            matrix->processBlock(block);
            break;
        }
        }
    }

    void prepare(dsp::ProcessSpec &spec)
    {
        softClipper->prepare(spec);
        fold->prepare(spec);
        patty->prepare(spec);
        fuzz->prepare(spec);
        tubeAmp->prepare(spec);
        phaseDist->prepare(spec);
        jeff->prepare(spec);
        diodeWaveshape->prepare(spec);
        rubidium->prepare(spec);
        matrix->prepare(spec);


        // init iir filter
        iirFilter.reset();
        *iirFilter.state = dsp::IIR::ArrayCoefficients<double>::makeFirstOrderHighPass(spec.sampleRate, 8.0f);
        iirFilter.prepare(spec);

        setSampleRate(spec.sampleRate);
    }

    void setSampleRate(float newSampleRate)
    {
        sampleRate = newSampleRate;
    }

private:
    // juce::AudioProcessorValueTreeState &treeStateRef;

    juce::AudioParameterChoice *distoType = nullptr;
    juce::AudioParameterBool *distortionEnabled;

    std::unique_ptr<SoftClip> softClipper = nullptr;
    std::unique_ptr<Cooked> fold = nullptr;
    std::unique_ptr<PattyFuzz> patty = nullptr;
    std::unique_ptr<Jeff> jeff = nullptr;
    std::unique_ptr<Fuzz> fuzz = nullptr;
    std::unique_ptr<DiodeWaveshape> diodeWaveshape = nullptr;
    std::unique_ptr<Amp> tubeAmp = nullptr;
    std::unique_ptr<PhaseDist> phaseDist = nullptr;
    std::unique_ptr<RubidiumDistortion> rubidium = nullptr;
    std::unique_ptr<MatrixWaveshaper> matrix = nullptr;

    dsp::ProcessorDuplicator<dsp::IIR::Filter<double>, dsp::IIR::Coefficients<double>> iirFilter;

    float sampleRate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrimaryDistortion)
};