#pragma once

#include "../Panel.h"

class StereoCompPanel : public Panel
{
public:
    StereoCompPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::dynamics, 0}) : Panel(p, "", Palette::colours[3]),
        threshold(p, "THRES", slot, ParamIDs::stereoCompThreshold, ValueType::db, ScopeContextType::COMPRESSION),
        ratio(p, "RATIO", slot, ParamIDs::compRatio, ValueType::compressionRatio, ScopeContextType::COMPRESSION),
        attack(p, "SPEED", slot, ParamIDs::compSpeed, ValueType::ms, ScopeContextType::COMPRESSION)
    {
        addKnobs({&threshold, &ratio, &attack});
    }

    void resized() override { layoutGrid({&threshold, &ratio, &attack}, 2); }

private:
    ParamKnob threshold, ratio, attack;
};
