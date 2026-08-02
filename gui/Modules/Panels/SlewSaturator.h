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
        type(p, "TYPE", ParamIDs::slewType, ParamUnits::category, ScopeContextType::IN_OUT)
    {
        addAndMakeVisible(bias);
        addAndMakeVisible(alpha);
        addAndMakeVisible(directionality);
        addAndMakeVisible(type);

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
        auto bounds = getLocalBounds();
        alpha.setBounds(bounds.removeFromTop(static_cast<int>(bounds.getHeight() * 0.6666f)).reduced(10));

        auto width = bounds.getWidth() / 3;
        bias.setBounds(bounds.removeFromLeft(width));
        directionality.setBounds(bounds.removeFromLeft(width));
        type.setBounds(bounds);
    }

    void parameterChanged(const juce::String& parameterID, float newValue) override
    {
        if (parameterID == ParamIDs::slewType.getParamID())
            makeBiasKnobTransparent();
    }

    void makeBiasKnobTransparent()
    {
        const bool isDisabled = static_cast<int>(type.knob.getValue()) != 0;

        juce::MessageManager::callAsync([this, isDisabled]() mutable {
            directionality.setAlpha(isDisabled ? 1.0f : 0.35f);
            directionality.setEnabled(isDisabled);
            directionality.repaint();
        });
    }

    AudioPluginAudioProcessor &apvts;

    ParamKnob alpha;
    ParamKnob bias;
    ParamKnob directionality;
    ParamKnob type;
};