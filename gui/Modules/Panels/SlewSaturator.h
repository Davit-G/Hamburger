#pragma once

#include "../Panel.h"
#include "../../SaturationIcons/CentredSVGIcon.h"

class SlewRatePanel : public Panel, private juce::AudioProcessorValueTreeState::Listener
{
public:
    SlewRatePanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::main, 0}) : Panel(p, "SLEW", Palette::colours[1]), processor(p),
        alpha(p, "ALPHA", slot, ParamIDs::alphaParam, ValueType::none, ScopeContextType::IN_OUT),
        bias(p, "TONE", slot, ParamIDs::slewSpeed, ValueType::none, ScopeContextType::IN_OUT),
        directionality(p, "BEND", slot, ParamIDs::directionality, ValueType::none, ScopeContextType::IN_OUT),
        type(p, "TYPE", ParamIDs::slewType, ValueType::category, ScopeContextType::IN_OUT),
        slewIcon(BinaryData::Slew_svg, BinaryData::Slew_svgSize, 3)
    {
        addKnobs({&bias, &alpha, &directionality, &type});
        addAndMakeVisible(slewIcon);

        p.treeState.addParameterListener(ParamIDs::slewType.getParamID(), this);

        makeBiasKnobTransparent();
    }

    ~SlewRatePanel() override {
        processor.treeState.removeParameterListener(ParamIDs::slewType.getParamID(), this);
    }

    void resized() override { layoutSaturation(alpha, slewIcon, {&bias, &type, &directionality}); }

    void parameterChanged(const juce::String& parameterID, float) override
    {
        if (parameterID == ParamIDs::slewType.getParamID())
            makeBiasKnobTransparent();
    }

    // bend only does something on the first slew type
    void makeBiasKnobTransparent()
    {
        const bool isDisabled = static_cast<int>(type.getValue()) != 0;

        juce::MessageManager::callAsync([this, isDisabled]() mutable {
            directionality.setAlpha(isDisabled ? 1.0f : 0.35f);
            directionality.setEnabled(isDisabled);
            directionality.repaint();
        });
    }

private:
    AudioPluginAudioProcessor &processor;

    ParamKnob alpha;
    RectSlider bias, directionality, type;
    CentredSVGIcon slewIcon;
};
