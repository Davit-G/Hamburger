#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"

class TypeAPanel : public Panel
{
public:
    TypeAPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::dynamics, 0}) : Panel(p, "TYPE A", &Theme::comp),
        threshold(p, "THRES", slot, ParamIDs::TypeAThreshold, ScopeContextType::COMPRESSION),
        speed(p, "SPEED", slot, ParamIDs::TypeACompSpeed, ScopeContextType::COMPRESSION),
        ratio(p, "RATIO", slot, ParamIDs::TypeARatio, ScopeContextType::COMPRESSION),
        tilt(p, "TILT", slot, ParamIDs::TypeATilt, ScopeContextType::COMPRESSION)
    {
        addKnobs({&threshold, &speed, &ratio, &tilt});
    }

    void resized() override { layoutGrid({&threshold, &ratio, &speed, &tilt}, 2); }

private:
    ParamKnob threshold, speed, ratio, tilt;
};
