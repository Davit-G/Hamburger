#pragma once

#include "../Panel.h"
#include "../../SaturationIcons/CentredSVGIcon.h"

class TubeSatPanel : public Panel
{
public:
    TubeSatPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::main, 0}) : Panel(p, "TUBE", Palette::colours[4]),
        tubeTone(p, "TUBE TONE", slot, ParamIDs::tubeTone, ValueType::none, ScopeContextType::IN_OUT),
        bias(p, "BIAS", slot, ParamIDs::tubeBias, ValueType::none, ScopeContextType::IN_OUT),
        jeff(p, "JEFF", slot, ParamIDs::jeffAmount, ValueType::none, ScopeContextType::IN_OUT),
        drive(p, "DRIVE", slot, ParamIDs::tubeAmount, ValueType::percent, ScopeContextType::IN_OUT),
        tube(BinaryData::Tube_svg, BinaryData::Tube_svgSize)
    {
        addKnobs({&tubeTone, &drive, &bias, &jeff});
        addAndMakeVisible(tube);
    }

    void resized() override { layoutSaturation(drive, tube, {&bias, &jeff, &tubeTone}); }

private:
    RectSlider tubeTone, bias, jeff;
    ParamKnob drive;
    CentredSVGIcon tube;
};
