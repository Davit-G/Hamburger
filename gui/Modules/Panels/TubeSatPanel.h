#pragma once

 
#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"
#include "../../Knob.h"




class TubeSatPanel : public Panel
{
public:
    TubeSatPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::main, 0}) : Panel(p, "TUBE", Palette::colours[4], slot), 
        tubeTone(p, "TUBE TONE", slot, ParamIDs::tubeTone, ScopeContextType::IN_OUT),
        bias(p, "BIAS", slot, ParamIDs::tubeBias, ScopeContextType::IN_OUT),
        jeff(p, "JEFF", slot, ParamIDs::jeffAmount, ScopeContextType::IN_OUT),
        drive(p, "DRIVE", slot, ParamIDs::tubeAmount, ScopeContextType::IN_OUT),
        tube(BinaryData::Tube_svg, BinaryData::Tube_svgSize)
    {
        addAndMakeVisible(tubeTone);
        addAndMakeVisible(drive);
        addAndMakeVisible(bias);
        addAndMakeVisible(jeff);
        addAndMakeVisible(tube);

        const auto colour = Palette::colours[4];
        Palette::setKnobColoursOfComponent(this, colour);

        Palette::setKnobColoursOfComponent(&tubeTone, colour);
        Palette::setKnobColoursOfComponent(&drive, colour);
        Palette::setKnobColoursOfComponent(&bias, colour);
        Palette::setKnobColoursOfComponent(&jeff, colour);
    }

    void resized() override
    {
        if (usesCompactLayout())
            compactLayout(drive, tube, {&bias, &jeff, &tubeTone});
        else
            fourKnobLayout(drive, tube, bias, jeff, tubeTone);
    }

    RectSlider tubeTone;
    ParamKnob drive;
    RectSlider jeff;
    RectSlider bias;

    CentredSVGIcon tube;
};