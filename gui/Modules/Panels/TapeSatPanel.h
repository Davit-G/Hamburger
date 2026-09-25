#pragma once
 
#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"
#include "../../Knob.h"

class TapeSatPanel : public Panel
{
public:
    TapeSatPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::main, 0}) : Panel(p, "TAPE", &Theme::tape, slot), 
        drive(p, "DRIVE", slot, ParamIDs::tapeDrive, ScopeContextType::IN_OUT),
        bias(p, "DC BIAS", slot, ParamIDs::tapeBias, ScopeContextType::IN_OUT),
        tapeWidth(p, "AGE", slot, ParamIDs::tapeWidth, ScopeContextType::IN_OUT),
        reel(BinaryData::FilmReel_svg, BinaryData::FilmReel_svgSize, &Theme::tape)
    {
        addAndMakeVisible(drive);
        addAndMakeVisible(bias);
        addAndMakeVisible(tapeWidth);
        addAndMakeVisible(reel);
    }

    void resized() override
    {
        if (usesCompactLayout())
            compactLayout(drive, reel, {&bias, &tapeWidth});
        else
            threeKnobLayout(drive, reel, bias, tapeWidth);
    }

    ParamKnob drive;
    RectSlider tapeWidth;
    RectSlider bias;

    CentredSVGIcon reel;
};