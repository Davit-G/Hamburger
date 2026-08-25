#pragma once
 
#include "../Panel.h"
#include "../../Knob.h"

class SlewRatePanel : public Panel, private juce::AudioProcessorValueTreeState::Listener
{
public:
    SlewRatePanel(AudioPluginAudioProcessor &p) : apvts(p), Panel(p, "SLEW"), 
        alpha(p, "ALPHA", ParamIDs::alphaParam, ParamUnits::none, ScopeContextType::IN_OUT),
        bias(p, "TONE", ParamIDs::slewSpeed, ParamUnits::none, ScopeContextType::IN_OUT),
        directionality(p, "BEND", ParamIDs::directionality, ParamUnits::none, ScopeContextType::IN_OUT),
        type(p, "TYPE", ParamIDs::slewType, ParamUnits::category, ScopeContextType::IN_OUT),
        slewIcon(BinaryData::Slew_svg, BinaryData::Slew_svgSize, 3)
    {
        addAndMakeVisible(bias);
        addAndMakeVisible(alpha);
        addAndMakeVisible(directionality);
        addAndMakeVisible(type);
        addAndMakeVisible(slewIcon);

        Palette::setKnobColoursOfComponent(&alpha, Palette::colours[1]);
        Palette::setKnobColoursOfComponent(&bias, Palette::colours[1]);
        Palette::setKnobColoursOfComponent(&type, Palette::colours[1]);
        Palette::setKnobColoursOfComponent(&directionality, Palette::colours[1]);

        p.treeState.addParameterListener(ParamIDs::slewType.getParamID(), this);

        makeBiasKnobTransparent();
    }

    ~SlewRatePanel() override {
        apvts.treeState.removeParameterListener(ParamIDs::slewType.getParamID(), this);
    }

    void resized() override
    {
        fourKnobLayout(alpha, slewIcon, bias, type, directionality);
    }

    void parameterChanged(const juce::String& parameterID, float newValue) override
    {
        if (parameterID == ParamIDs::slewType.getParamID())
            makeBiasKnobTransparent();
    }

    void makeBiasKnobTransparent()
    {
        const bool isDisabled = static_cast<int>(type.getValue()) != 0;

        juce::MessageManager::callAsync([this, isDisabled]() mutable {
            directionality.setAlpha(isDisabled ? 1.0f : 0.35f);
            directionality.setEnabled(isDisabled);
            directionality.repaint();
        });
    }

    AudioPluginAudioProcessor &apvts;

    ParamKnob alpha;
    RectSlider bias;
    RectSlider directionality;
    RectSlider type;

    CentredSVGIcon slewIcon;
};