#pragma once

#include "../Panel.h"


class OtherUtils : public Panel
{
public:
    OtherUtils(AudioPluginAudioProcessor &p) : Panel(p, "OTHER?"),
    quality(p, "OVERSMPL", ParamIDs::oversamplingFactor, ParamUnits::x),
    stages(p, "STAGES", ParamIDs::stages, ParamUnits::x)
    {
        addAndMakeVisible(quality);
        addAndMakeVisible(stages);
    }

    void resized() {
        auto bounds = getLocalBounds();

        quality.setBounds(bounds.removeFromLeft(bounds.getWidth() / 2));
        stages.setBounds(bounds);
    }
private:
    Grid grid;
    ParamKnob quality;
    ParamKnob stages;
};