#pragma once

#include "../Panel.h"


class OtherUtils : public Panel
{
public:
    OtherUtils(AudioPluginAudioProcessor &p) : Panel(p, "OTHER?"),
    quality(p, "OVERSMPL", ParamIDs::oversamplingFactor)
    {
        addAndMakeVisible(quality);
    }

    void resized() {
        quality.setBounds(getLocalBounds());
    }
private:
    ParamKnob quality;
};
