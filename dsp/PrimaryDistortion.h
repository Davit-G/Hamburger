#pragma once

#include "Distortions/Cooked.h"
#include "Distortions/DiodeWaveshape.h"
#include "Distortions/Fuzz.h"
#include "Distortions/PattyFuzz.h"
#include "Distortions/PhaseDist.h"
#include "Distortions/Rubidium.h"
#include "Distortions/SoftClipper.h"
#include "Distortions/nonlinslew/NonlinSlew.h"
#include "Distortions/preisach/Preisach.h"

#include "DCBlockingHighPass.h"

#include "./Noise/Jeff.h"
#include "Distortions/tube/Amp.h"

#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_core/juce_core.h"
#include "juce_dsp/juce_dsp.h"

#include "EffectBase.h"
#include "EffectInfos.h"

// #include <melatonin_perfetto/melatonin_perfetto.h>

class PrimaryDistortion : public EffectBase {
  public:
    PrimaryDistortion(juce::AudioProcessorValueTreeState &state,
                      SlotId slot = SlotId{ModuleId::main, 0})
        : slot(slot) {
        distoType = dynamic_cast<juce::AudioParameterChoice *>(
            state.getParameter(slot.type().getParamID()));
        jassert(distoType);

        distortionEnabled = dynamic_cast<juce::AudioParameterBool *>(
            state.getParameter(slot.enabled().getParamID()));
        jassert(distortionEnabled);

        // GRILL
        softClipper =
            std::make_unique<SoftClip>(state, slot);
        fuzz = std::make_unique<Fuzz>(state, slot);
        diodeWaveshape =
            std::make_unique<DiodeWaveshape>(state, slot);
        fold = std::make_unique<Cooked>(state, slot);
        patty = std::make_unique<PattyFuzz>(state, slot);

        // TUBE
        jeff = std::make_unique<Jeff>(state, slot);
        tubeAmp = std::make_unique<Amp>(state, slot);

        // one algorithm per type
        phaseDist =
            std::make_unique<PhaseDist>(state, slot);
        rubidium = std::make_unique<RubidiumDistortion>(
            state, slot);
        preisach = std::make_unique<Preisach>(state, slot);
        nonlinSlew =
            std::make_unique<NonlinSlew>(state, slot);
    }

    ~PrimaryDistortion() {}

    void prepare(juce::dsp::ProcessSpec &spec) override {
        softClipper->prepare(spec);
        fold->prepare(spec);
        patty->prepare(spec);
        fuzz->prepare(spec);
        tubeAmp->prepare(spec);
        phaseDist->prepare(spec);
        jeff->prepare(spec);
        diodeWaveshape->prepare(spec);
        rubidium->prepare(spec);
        preisach->prepare(spec);
        nonlinSlew->prepare(spec);

        setSampleRate(spec.sampleRate);

        previousDistType = -1;

        for (int i = 0; i < 3; i++) {
            dcBlocker[i].prepare(spec);
        }

        emptyBuffer = juce::AudioBuffer<float>(spec.numChannels, 8192);
        emptyBlock = juce::dsp::AudioBlock<float>(emptyBuffer);

        fillEmptyWithZeros();
        processBlock(emptyBlock);
    }

    void fillEmptyWithZeros() { emptyBlock.fill(0.0f); }


    void processBlock(juce::dsp::AudioBlock<float> &block) override {
        int distoTypeIndex = distoType->getIndex();

        if (previousDistType != distoTypeIndex) {
            previousDistType = distoTypeIndex;


            // declicking attempt
            fillEmptyWithZeros();
            processBlock(emptyBlock);
        }

        if (distortionEnabled->get() == false)
            return;

        switch (distoTypeIndex) {
        case 0: { // classic
            // TRACE_EVENT("dsp", "classic");
            fuzz->processBlock(block);
            fold->processBlock(block);
            diodeWaveshape->processBlock(block);
            softClipper->processBlock(block);
            dcBlocker[0].processBlock(block);
            break;
        }
        case 1: { // tube
            // TRACE_EVENT("dsp", "tube");
            jeff->processBlock(block);
            tubeAmp->processBlock(block);
            break;
        }
        case 2: { // phase distortion
            // TRACE_EVENT("dsp", "phase");
            phaseDist->processBlock(block);
            break;
        }
        case 3: { // rubidium distortion
            // TRACE_EVENT("dsp", "rubidium");
            rubidium->processBlock(block);
            break;
        }
        case 4: { // tape hysteresis
            // TRACE_EVENT("dsp", "tape");
            preisach->processBlock(block);
            dcBlocker[1].processBlock(block);
            break;
        }
        case 5: {
            nonlinSlew->processBlock(block);
            dcBlocker[2].processBlock(block);
            break;
        }
        default:
            break;
        }
    }

    void setSampleRate(float newSampleRate) { sampleRate = newSampleRate; }

  private:
    // juce::AudioProcessorValueTreeState &treeStateRef;

    juce::AudioBuffer<float> emptyBuffer;
    juce::dsp::AudioBlock<float> emptyBlock;

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
    std::unique_ptr<Preisach> preisach = nullptr;
    std::unique_ptr<NonlinSlew> nonlinSlew = nullptr;

    DCBlockingHighPass dcBlocker[3];

    SlotId slot;

    int previousDistType = -1;

    float sampleRate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrimaryDistortion)
};