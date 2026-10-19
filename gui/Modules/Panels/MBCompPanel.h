#pragma once

#include "../Panel.h"

class MBCompPanel : public Panel
{
public:
    MBCompPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::dynamics, 0}) : Panel(p, "MB", Palette::colours[3]),
        threshold(p, "THRES", slot, ParamIDs::MBCompThreshold, ValueType::db, ScopeContextType::COMPRESSION),
        ratio(p, "RATIO", slot, ParamIDs::compRatio, ValueType::compressionRatio, ScopeContextType::COMPRESSION),
        tilt(p, "TILT", slot, ParamIDs::compBandTilt, ValueType::db, ScopeContextType::COMPRESSION),
        attack(p, "SPEED", slot, ParamIDs::compSpeed, ValueType::ms, ScopeContextType::COMPRESSION)
    {
        addKnobs({&threshold, &ratio, &tilt, &attack});
    }

    void resized() override { layoutGrid({&threshold, &tilt, &ratio, &attack}, 3); }

private:
    ParamKnob threshold, ratio, tilt, attack;
};
