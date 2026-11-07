#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"

class MBCompPanel : public Panel
{
public:
    MBCompPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::dynamics, 0}) : Panel(p, "MB", &Theme::comp),
        threshold(p, "THRES", slot, ParamIDs::MBCompThreshold, ScopeContextType::COMPRESSION),
        ratio(p, "RATIO", slot, ParamIDs::compRatio, ScopeContextType::COMPRESSION),
        tilt(p, "TILT", slot, ParamIDs::compBandTilt, ScopeContextType::COMPRESSION),
        attack(p, "SPEED", slot, ParamIDs::MBCompSpeed, ScopeContextType::COMPRESSION)
    {
        addKnobs({&threshold, &ratio, &tilt, &attack});
    }

    void resized() override { layoutGrid({&threshold, &ratio, &attack, &tilt}, 2); }

private:
    ParamKnob threshold, ratio, tilt, attack;
};
