#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"
#include "../../SaturationIcons/CentredSVGIcon.h"

class TapeSatPanel : public Panel
{
public:
    TapeSatPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::main, 0}) : Panel(p, "TAPE", &Theme::tape, slot),
        drive(p, "DRIVE", slot, ParamIDs::tapeDrive, ScopeContextType::IN_OUT),
        bias(p, "DC BIAS", slot, ParamIDs::tapeBias, ScopeContextType::IN_OUT),
        tapeWidth(p, "AGE", slot, ParamIDs::tapeWidth, ScopeContextType::IN_OUT),
        reel(BinaryData::FilmReel_svg, BinaryData::FilmReel_svgSize, &Theme::tape)
    {
        addKnobs({&drive, &bias, &tapeWidth});
        addAndMakeVisible(reel);
    }

    void resized() override { layoutSaturation(drive, reel, {&bias, &tapeWidth}); }

private:
    ParamKnob drive;
    RectSlider bias, tapeWidth;
    CentredSVGIcon reel;
};
