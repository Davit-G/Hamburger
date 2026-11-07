#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"

// the level and which step of the pitch comes out
class HarmonicPanel : public Panel
{
public:
    explicit HarmonicPanel(AudioPluginAudioProcessor &p) : Panel(p, "HARMONIC", &Theme::preFx),
        amount(p, "AMOUNT", SlotId{ModuleId::module2, 0}, ParamIDs::harmonicAmount),
        ratio(p, "RATIO", SlotId{ModuleId::module2, 0}, ParamIDs::harmonicRatio)
    {
        addKnobs({&amount, &ratio});
    }

    void resized() override { layoutGrid({&amount, &ratio}); }

private:
    ParamKnob amount, ratio;
};

class FormantShiftPanel : public Panel
{
public:
    explicit FormantShiftPanel(AudioPluginAudioProcessor &p) : Panel(p, "FORMANT", &Theme::preFx),
        shift(p, "SHIFT", SlotId{ModuleId::module2, 0}, ParamIDs::formantShift),
        depth(p, "DEPTH", SlotId{ModuleId::module2, 0}, ParamIDs::formantDepth),
        resonance(p, "RESO",SlotId{ModuleId::module2, 0}, ParamIDs::formantResonance)
    {
        addKnobs({&shift, &depth, &resonance});
    }

    void resized() override { layoutGrid({&shift, &depth, &resonance}); }

private:
    ParamKnob shift, depth, resonance;
};

class FreqShiftPanel : public Panel
{
public:
    explicit FreqShiftPanel(AudioPluginAudioProcessor &p) : Panel(p, "FREQ SHIFT", &Theme::preFx),
        shift(p, "SHIFT", SlotId{ModuleId::module2, 0}, ParamIDs::freqShift),
        feedback(p, "FEEDBACK", SlotId{ModuleId::module2, 0}, ParamIDs::freqShiftFeedback),
        delay(p, "DELAY", SlotId{ModuleId::module2, 0}, ParamIDs::freqShiftDelay)
    {
        addKnobs({&shift, &feedback, &delay});
    }

    void resized() override { layoutGrid({&shift, &feedback, &delay}); }

private:
    ParamKnob shift, feedback, delay;
};
