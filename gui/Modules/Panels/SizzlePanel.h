#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"

class SizzlePanel : public Panel
{
public:
    SizzlePanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module1, 0}) : Panel(p, "SIZZLE", &Theme::noise),
        sizzleKnob(p, "SIZZLE", slot, ParamIDs::sizzleAmount, ScopeContextType::NOISE),
        sizzleFreq(p, "FREQ", slot, ParamIDs::sizzleFrequency, ScopeContextType::NOISE),
        sizzleQ(p, "Q", slot, ParamIDs::sizzleQ, ScopeContextType::NOISE)
    {
        addKnobs({&sizzleKnob, &sizzleFreq, &sizzleQ});
    }

    void resized() override { layoutGrid({&sizzleKnob, &sizzleFreq, &sizzleQ}); }

private:
    ParamKnob sizzleKnob, sizzleFreq, sizzleQ;
};

class SizzleOGPanel : public Panel
{
public:
    SizzleOGPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module1, 0}) : Panel(p, "FIZZ", &Theme::noise),
        sizzleKnob(p, "FIZZLE", slot, ParamIDs::fizzAmount, ScopeContextType::NOISE)
    {
        addKnobs({&sizzleKnob});
    }

    void resized() override { layoutGrid({&sizzleKnob}); }

private:
    ParamKnob sizzleKnob;
};
