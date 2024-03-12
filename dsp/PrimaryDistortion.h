#pragma once

#include "Distortions/SoftClipper.h"
#include "Distortions/PattyFuzz.h"
#include "Distortions/Fuzz.h"
#include "Distortions/Cooked.h"
#include "Distortions/PhaseDist.h"

#include "Distortions/tube/Amp.h"

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
        fuzz = std::make_unique<Fuzz>(state);
        tubeAmp = std::make_unique<Amp>(state);
        phaseDist = std::make_unique<PhaseDist>(state);
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
            patty->processBlock(block);
            fuzz->processBlock(block);
            softClipper->processBlock(block);
            iirFilter.process(context); // hpf afterwards to remove bias
            break;
        }
        case 1:
        { // tube
            TRACE_EVENT("dsp", "tube");
            tubeAmp->processBlock(block);
            break;
        }
        // case 2:
        // { // waveshaper style
        //     TRACE_EVENT("dsp", "waveshaper");
        //     break;
        // }
        case 2:
        { // phase distortion
            TRACE_EVENT("dsp", "phase");
            phaseDist->processBlock(block);
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

        // init iir filter
        iirFilter.reset();
        *iirFilter.state = dsp::IIR::ArrayCoefficients<float>::makeHighPass(spec.sampleRate, 5.0f, 0.707f);
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
    std::unique_ptr<Fuzz> fuzz = nullptr;
    std::unique_ptr<Amp> tubeAmp = nullptr;
    std::unique_ptr<PhaseDist> phaseDist = nullptr;

    dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> iirFilter;

    float sampleRate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrimaryDistortion)
};