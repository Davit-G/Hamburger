#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"
#include "../../SaturationIcons/CentredSVGIcon.h"

class SlewRatePanel : public Panel
{
public:
    SlewRatePanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::main, 0}) : Panel(p, "SLEW", &Theme::slew, slot),
        drive(p, "DRIVE", slot, ParamIDs::alphaParam, ScopeContextType::IN_OUT),
        tone(p, "TONE", slot, ParamIDs::slewSpeed, ScopeContextType::IN_OUT),
        bias(p, "BIAS", slot, ParamIDs::directionality, ScopeContextType::IN_OUT),
        type(p, "TYPE", slot, ParamIDs::slewType, ScopeContextType::IN_OUT),
        slewIcon(BinaryData::Slew_svg, BinaryData::Slew_svgSize, &Theme::slew, 3)
    {
        addKnobs({&tone, &drive, &bias, &type});
        addAndMakeVisible(slewIcon);
    }

    void resized() override { layoutSaturation(drive, slewIcon, {&tone, &type, &bias}); }

private:
    ParamKnob drive;
    RectSlider tone, bias, type;
    CentredSVGIcon slewIcon;
};
