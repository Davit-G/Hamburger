#pragma once

#include "../Panel.h"

class SizzlePanel : public Panel
{
public:
    SizzlePanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module1, 0}) : Panel(p, "SIZZLE", Palette::colours[1]),
        sizzleKnob(p, "SIZZLE", slot, ParamIDs::sizzleAmount, ValueType::none, ScopeContextType::NOISE),
        sizzleFreq(p, "FREQ", slot, ParamIDs::sizzleFrequency, ValueType::none, ScopeContextType::NOISE),
        sizzleQ(p, "Q", slot, ParamIDs::sizzleQ, ValueType::none, ScopeContextType::NOISE)
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
    SizzleOGPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module1, 0}) : Panel(p, "FIZZ"),
        sizzleKnob(p, "FIZZLE", slot, ParamIDs::fizzAmount, ValueType::none, ScopeContextType::NOISE)
    {
        addKnobs({&sizzleKnob});
    }

    void resized() override { layoutGrid({&sizzleKnob}); }

private:
    ParamKnob sizzleKnob;
};
