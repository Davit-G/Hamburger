#pragma once

#include "../Panel.h"
#include "../../SaturationIcons/CentredSVGIcon.h"

class RubidiumSatPanel : public Panel
{
public:
    RubidiumSatPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::main, 0}) : Panel(p, "RUBIDIUM", Palette::colours[2]),
        tone(p, "TONE", slot, ParamIDs::rubidiumTone, ValueType::hz, ScopeContextType::IN_OUT),
        mojo(p, "MOJO", slot, ParamIDs::rubidiumMojo, ValueType::none, ScopeContextType::IN_OUT),
        hysteresis(p, "ASYM", slot, ParamIDs::rubidiumAsym, ValueType::none, ScopeContextType::IN_OUT),
        drive(p, "DRIVE", slot, ParamIDs::rubidiumAmount, ValueType::percent, ScopeContextType::IN_OUT),
        bias(p, "BIAS", slot, ParamIDs::rubidiumBias, ValueType::none, ScopeContextType::IN_OUT),
        flask(BinaryData::Flask_svg, BinaryData::Flask_svgSize)
    {
        addKnobs({&tone, &drive, &mojo, &hysteresis, &bias});
        addAndMakeVisible(flask);
    }

    void resized() override { layoutSaturation(drive, flask, {&bias, &hysteresis, &mojo, &tone}); }

private:
    RectSlider tone, mojo, hysteresis;
    ParamKnob drive;
    RectSlider bias;
    CentredSVGIcon flask;
};
