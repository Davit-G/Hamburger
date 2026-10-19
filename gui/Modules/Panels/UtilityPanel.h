#pragma once

#include "../Panel.h"

class UtilityPanel : public Panel
{
public:
    UtilityPanel(AudioPluginAudioProcessor &p) : Panel(p, "UTILITY", juce::Colours::whitesmoke),
        inGain(p, "IN", ParamIDs::inputGain, ValueType::db),
        mix(p, "MIX", ParamIDs::mix, ValueType::percent),
        outGain(p, "OUT", ParamIDs::outputGain, ValueType::db)
    {
        addKnobs({&inGain, &mix, &outGain});
    }

    void resized() override { layoutGrid({&inGain, &mix, &outGain}); }

private:
    ParamKnob inGain, mix, outGain;
};
