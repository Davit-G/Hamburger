#pragma once

#include "../Panel.h"
#include "PanelNames.h"

class OtherUtils : public Panel
{
public:
    OtherUtils(AudioPluginAudioProcessor &p) : Panel(p, "OTHER?"),
    quality(p, "QUALITY FACTOR", "oversamplingFactor", ParamUnits::x)
    {
        addAndMakeVisible(quality);
    }

    void resized() {
        auto bounds = getLocalBounds();

        quality.setBounds(bounds);
    }
private:
    Grid grid;
    ParamKnob quality;
};