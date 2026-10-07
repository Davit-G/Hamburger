#pragma once

#include "../Panel.h"


class OtherUtils : public Panel
{
public:
    OtherUtils(AudioPluginAudioProcessor &p) : Panel(p, "OTHER?"),
    quality(p, "OVERSMPL", ParamIDs::oversamplingFactor, ValueType::oversample)
    {
        addAndMakeVisible(quality);
    }

    void resized() {
        auto bounds = getLocalBounds();

        quality.setBounds(bounds);
    }
private:
    juce::Grid grid;
    ParamKnob quality;
};