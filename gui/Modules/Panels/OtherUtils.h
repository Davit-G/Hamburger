#pragma once

#include "../Panel.h"


class OtherUtils : public Panel
{
public:
    OtherUtils(AudioPluginAudioProcessor &p) : Panel(p, "OTHER?"),
    quality(p, "QUALITY FACTOR", ParamIDs::oversamplingFactor, ParamUnits::x)
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