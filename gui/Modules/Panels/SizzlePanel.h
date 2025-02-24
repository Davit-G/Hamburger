#pragma once

#include "../Panel.h"


class SizzlePanel : public Panel
{
public:
    SizzlePanel(AudioPluginAudioProcessor &p) : Panel(p, "SIZZLE"),
    sizzleKnob(p, "SIZZLE", ParamIDs::sizzleAmount),
    sizzleFreq(p, "FREQ", ParamIDs::sizzleFrequency),
    sizzleQ(p, "Q", ParamIDs::sizzleQ) {
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
    sizzleKnob(p, "FIZZLE", ParamIDs::fizzAmount)
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