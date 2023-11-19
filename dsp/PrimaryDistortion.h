#pragma once

#include <JuceHeader.h>

#include "Distortions/SoftClipper.h"
#include "Distortions/HardClip.h"
#include "Distortions/PattyFuzz.h"
#include "Distortions/Fuzz.h"
#include "Distortions/Cooked.h"
#include "Distortions/Grunge.h"

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
        hardClipper = std::make_unique<HardClip>(state);
        fold = std::make_unique<Cooked>(state);
        patty = std::make_unique<PattyFuzz>(state);
        fuzz = std::make_unique<Fuzz>(state);
        grunge = std::make_unique<Grunge>(state);
        tubeAmp = std::make_unique<Amp>(state);
    }

    ~PrimaryDistortion() {}

    void processBlock(dsp::AudioBlock<float> &block)
    {
        TRACE_DSP();
        int distoTypeIndex = distoType->getIndex();

        if (distortionEnabled->get() == false)
            return;

        switch (distoTypeIndex)
        {
        case 0: {// classic
            // fold->processBlock(block);
            // patty->processBlock(block);

            // TRACE_EVENT_BEGIN("dsp", "classic");
            TRACE_EVENT("dsp", "classic");
            grunge->processBlock(block);
            patty->processBlock(block);
            fuzz->processBlock(block);
            softClipper->processBlock(block);
            iirFilter.process(dsp::ProcessContextReplacing<float>(block)); // hpf afterwards to remove bias
            // TRACE_EVENT_END("dsp", "classic");
            break;
        }
        case 1: {// tube
            // TRACE_EVENT_BEGIN("dsp", "tube");
            TRACE_EVENT("dsp", "tube");
            grunge->processBlock(block);
            tubeAmp->processBlock(block);
            // TRACE_EVENT_END("dsp", "tube");
            break;
        }
        case 2: // wavefolding
            break;
        case 3: // fuzz
            break;
        case 4: // tube
            break;
        case 5:
            break;
        default:
            break;
        }
    }

    void prepare(dsp::ProcessSpec& spec)
    {
        setSampleRate(sampleRate);
        softClipper->prepare(spec);
        hardClipper->prepare(spec);
        fold->prepare(spec);
        patty->prepare(spec);
        fuzz->prepare(spec);
        grunge->prepare(spec);
        tubeAmp->prepare(spec);

        // init iir filter
        *iirFilter.state = *dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 5.0f, 0.707f);

        iirFilter.prepare(spec);
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
    std::unique_ptr<HardClip> hardClipper = nullptr;
    std::unique_ptr<Cooked> fold = nullptr;
    std::unique_ptr<PattyFuzz> patty = nullptr;
    std::unique_ptr<Fuzz> fuzz = nullptr;
    std::unique_ptr<Grunge> grunge = nullptr;
    std::unique_ptr<Amp> tubeAmp = nullptr;

    dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> iirFilter;

    float sampleRate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrimaryDistortion)
};