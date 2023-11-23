#pragma once

#include <JuceHeader.h>
#include "../Panel.h"
#include "../../Knob.h"
#include "PanelNames.h"

class TubeSatPanel : public Panel
{
public:
    TubeSatPanel(AudioPluginAudioProcessor &p) : Panel(p, "TUBE"), 
        tubeTone(p, "TUBE TONE", "tubeTone"), 
        grungeAmt(p, "GRUNGE", "grungeAmt"),
        grungeTone(p, "GRUNGE TONE", "grungeTone"), 
        drive(p, "DRIVE", "saturationAmount", ParamUnits::percent)
        
    {
        auto laf = new KnobLAF(juce::Colours::yellowgreen);
        setLookAndFeel(laf); // will cascade to all children knobs

        addAndMakeVisible(tubeTone);
        addAndMakeVisible(grungeTone);
        addAndMakeVisible(grungeAmt);
        addAndMakeVisible(drive);
    }

    // void paint(juce::Graphics &g) override
    // {
    //     // g.fillAll(juce::Colours::green);
    // }

    void resized() override
    {
        auto bounds = getLocalBounds();
        drive.setBounds(bounds.removeFromTop(bounds.getHeight() / 1.5f).reduced(10));

        auto width = bounds.getWidth() / 3;
        grungeAmt.setBounds(bounds.removeFromLeft(width));
        grungeTone.setBounds(bounds.removeFromLeft(width));
        tubeTone.setBounds(bounds.removeFromLeft(width));
    }

    ParamKnob tubeTone;
    ParamKnob grungeTone;
    ParamKnob grungeAmt;
    ParamKnob drive;
};