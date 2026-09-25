#pragma once

 
#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"
#include "../../Knob.h"



class RubidiumSatPanel : public Panel
{
public:
    RubidiumSatPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::main, 0}) : Panel(p, "RUBIDIUM", &Theme::rubidium, slot), 
        tone(p, "TONE", slot, ParamIDs::rubidiumTone, ScopeContextType::IN_OUT),
        mojo(p, "MOJO", slot, ParamIDs::rubidiumMojo, ScopeContextType::IN_OUT),
        hysteresis(p, "ASYM", slot, ParamIDs::rubidiumAsym, ScopeContextType::IN_OUT),
        drive(p, "DRIVE", slot, ParamIDs::rubidiumAmount, ScopeContextType::IN_OUT),
        bias(p, "BIAS", slot, ParamIDs::rubidiumBias, ScopeContextType::IN_OUT),
        flask(BinaryData::Flask_svg, BinaryData::Flask_svgSize, &Theme::rubidium)
    {
        addAndMakeVisible(tone);
        addAndMakeVisible(drive);
        addAndMakeVisible(mojo);
        addAndMakeVisible(hysteresis);
        addAndMakeVisible(bias);
        addAndMakeVisible(flask);
    }

    void resized() override
    {
        if (usesCompactLayout())
            compactLayout(drive, flask, {&bias, &hysteresis, &mojo, &tone});
        else
            fiveKnobLayout(drive, flask, bias, hysteresis, mojo, tone);
    }

    ParamKnob drive;
    RectSlider tone;
    RectSlider mojo;
    RectSlider hysteresis;
    RectSlider bias;

    CentredSVGIcon flask;
};