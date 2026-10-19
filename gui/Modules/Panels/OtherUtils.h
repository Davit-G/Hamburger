#pragma once

#include "../Panel.h"

class OtherUtils : public Panel
{
public:
    OtherUtils(AudioPluginAudioProcessor &p) : Panel(p, "OTHER?"),
        quality(p, "OVERSMPL", ParamIDs::oversamplingFactor, ValueType::oversample)
    {
        addKnobs({&quality});
    }

    void resized() override { layoutGrid({&quality}); }

private:
    ParamKnob quality;
};
