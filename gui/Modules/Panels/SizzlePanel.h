#pragma once

 

#include "../Panel.h"
#include "PanelNames.h"

class SizzlePanel : public Panel
{
public:
    SizzlePanel(AudioPluginAudioProcessor &p) : Panel(p, "SIZZLE"),
    sizzleKnob(p, "SIZZLE", "noiseAmount"),
    sizzleFreq(p, "FREQ", "noiseFrequency"),
    sizzleQ(p, "Q", "noiseQ") {
        addAndMakeVisible(sizzleKnob);
        addAndMakeVisible(sizzleFreq);
        addAndMakeVisible(sizzleQ);
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
    sizzleKnob(p, "FIZZLE", "noiseAmount")
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