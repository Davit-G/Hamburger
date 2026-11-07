#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"
#include "../../Knob.h"
#include "../../RectSlider.h"


// the opto compressors, where the audio only lights the cells, in one band with a stereo link or three with a threshold tilt
class OptoCompPanel : public Panel
{
public:
    OptoCompPanel(AudioPluginAudioProcessor &p, bool multiband)
        : Panel(p, multiband ? "MB OPTO" : "OPTO", multiband ? &Theme::comp : &Theme::comp),
        threshold(p, "THRES", compSlot, ParamIDs::optoThreshold, ScopeContextType::COMPRESSION),
        ratio(p, "RATIO", compSlot, ParamIDs::optoRatio, ScopeContextType::COMPRESSION),
        speed(p, "SPEED", compSlot, ParamIDs::optoCompSpeed, ScopeContextType::COMPRESSION),
        linkOrTilt(p, multiband ? "TILT" : "LINK", compSlot, multiband ? ParamIDs::compBandTilt : ParamIDs::compStereoLink, ScopeContextType::COMPRESSION)
    {
        addKnobs({&threshold, &ratio, &speed, &linkOrTilt});
    }

    void resized() override { layoutGrid({&threshold, &ratio, &speed, &linkOrTilt}, 2); }

private:
    static constexpr SlotId compSlot { ModuleId::dynamics, 0 };

    ParamKnob threshold, ratio, speed, linkOrTilt;
};
