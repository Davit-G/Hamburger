#pragma once

#include "../Panel.h"


class SizzlePanel : public Panel
{
public:
    SizzlePanel(AudioPluginAudioProcessor &p) : Panel(p, "SIZZLE"),
    sizzleKnob(p, "SIZZLE", ParamIDs::sizzleAmount, ParamUnits::none, ScopeContextType::NOISE),
    sizzleFreq(p, "FREQ", ParamIDs::sizzleFrequency, ParamUnits::none, ScopeContextType::NOISE),
    sizzleQ(p, "Q", ParamIDs::sizzleQ, ParamUnits::none, ScopeContextType::NOISE) {
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
    SizzleOGPanel(AudioPluginAudioProcessor &p) : Panel(p, "FIZZ"),
    sizzleKnob(p, "FIZZLE", ParamIDs::fizzAmount, ParamUnits::none, ScopeContextType::NOISE)
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