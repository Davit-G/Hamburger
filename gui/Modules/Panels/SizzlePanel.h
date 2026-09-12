#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"


class SizzlePanel : public Panel
{
public:
    SizzlePanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module1, 0}) : Panel(p, "SIZZLE"),
    sizzleKnob(p, "SIZZLE", slot, ParamIDs::sizzleAmount, ScopeContextType::NOISE),
    sizzleFreq(p, "FREQ", slot, ParamIDs::sizzleFrequency, ScopeContextType::NOISE),
    sizzleQ(p, "Q", slot, ParamIDs::sizzleQ, ScopeContextType::NOISE) {
        addAndMakeVisible(sizzleKnob);
        addAndMakeVisible(sizzleFreq);
        addAndMakeVisible(sizzleQ);

        Palette::setKnobColoursOfComponent(&sizzleKnob, Palette::colours[1]);
        Palette::setKnobColoursOfComponent(&sizzleFreq, Palette::colours[1]);
        Palette::setKnobColoursOfComponent(&sizzleQ, Palette::colours[1]);
        
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto width = bounds.getWidth() / 3;
        sizzleKnob.setBounds(bounds.removeFromLeft(width));
        sizzleFreq.setBounds(bounds.removeFromLeft(width));
        sizzleQ.setBounds(bounds);
    }

private:
    ParamKnob sizzleKnob;
    ParamKnob sizzleFreq;
    ParamKnob sizzleQ;
};

class SizzleOGPanel : public Panel
{
public:
    SizzleOGPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::module1, 0}) : Panel(p, "FIZZ"),
    sizzleKnob(p, "FIZZLE", slot, ParamIDs::fizzAmount, ScopeContextType::NOISE)
    {
        addAndMakeVisible(sizzleKnob);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        sizzleKnob.setBounds(bounds);
    }

private:
    ParamKnob sizzleKnob;
};