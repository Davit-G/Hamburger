#pragma once

#include "../Panel.h"

class TypeAPanel : public Panel
{
public:
    TypeAPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::dynamics, 0}) : Panel(p, "TYPE A", Palette::colours[3]),
        threshold(p, "THRES", slot, ParamIDs::TypeAThreshold, ValueType::db, ScopeContextType::COMPRESSION),
        speed(p, "SPEED", slot, ParamIDs::compSpeed, ValueType::ms, ScopeContextType::COMPRESSION),
        tilt(p, "TILT", slot, ParamIDs::TypeATilt, ValueType::db, ScopeContextType::COMPRESSION)
    {
        addKnobs({&threshold, &speed, &tilt});
    }

    void resized() override { layoutGrid({&threshold, &tilt, &speed}, 2); }

private:
    ParamKnob threshold, speed, tilt;
};
