#pragma once

#include "../Panel.h"
#include "../../SaturationIcons/CentredSVGIcon.h"

class TapeSatPanel : public Panel
{
public:
    TapeSatPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::main, 0}) : Panel(p, "TAPE", Palette::colours[7]),
        drive(p, "DRIVE", slot, ParamIDs::tapeDrive, ValueType::none, ScopeContextType::IN_OUT),
        bias(p, "DC BIAS", slot, ParamIDs::tapeBias, ValueType::none, ScopeContextType::IN_OUT),
        tapeWidth(p, "AGE", slot, ParamIDs::tapeWidth, ValueType::none, ScopeContextType::IN_OUT),
        reel(BinaryData::FilmReel_svg, BinaryData::FilmReel_svgSize)
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
