#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"

class MSCompPanel : public Panel
{
public:
    MSCompPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::dynamics, 0}) : Panel(p, "MS", &Theme::comp),
        threshold(p, "THRES", slot, ParamIDs::MSCompThreshold, ScopeContextType::COMPRESSION),
        ratio(p, "RATIO", slot, ParamIDs::compRatio, ScopeContextType::COMPRESSION),
        tilt(p, "TILT", slot, ParamIDs::compBandTilt, ScopeContextType::COMPRESSION),
        attack(p, "SPEED", slot, ParamIDs::MSCompSpeed, ScopeContextType::COMPRESSION)
    {
        addKnobs({&threshold, &ratio, &tilt, &attack});
    }

    void resized() override { layoutGrid({&threshold, &ratio, &attack, &tilt}, 2); }

private:
    ParamKnob threshold, ratio, tilt, attack;
};
