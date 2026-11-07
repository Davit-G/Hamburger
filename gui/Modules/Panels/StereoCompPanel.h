#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"

class StereoCompPanel : public Panel
{
public:
    StereoCompPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::dynamics, 0}) : Panel(p, "STEREO", &Theme::comp),
        threshold(p, "THRES", slot, ParamIDs::stereoCompThreshold, ScopeContextType::COMPRESSION),
        ratio(p, "RATIO", slot, ParamIDs::compRatio, ScopeContextType::COMPRESSION),
        attack(p, "SPEED", slot, ParamIDs::compSpeed, ScopeContextType::COMPRESSION),
        link(p, "LINK", slot, ParamIDs::compStereoLink, ScopeContextType::COMPRESSION)
    {
        addKnobs({&threshold, &ratio, &attack, &link});
    }

    void resized() override { layoutGrid({&threshold, &ratio, &attack, &link}, 2); }

private:
    ParamKnob threshold, ratio, attack, link;
};
