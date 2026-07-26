#pragma once
 
#include "../Panel.h"
#include "../../Knob.h"

class SlewRatePanel : public Panel, private juce::AudioProcessorValueTreeState::Listener
{
public:
    SlewRatePanel(AudioPluginAudioProcessor &p) : Panel(p, "SLEW"), 
        alpha(p, "ALPHA", ParamIDs::alphaParam),
        bias(p, "TONE", ParamIDs::slewSpeed),
        directionality(p, "BEND", ParamIDs::directionality),
        type(p, "TYPE", ParamIDs::slewType, ParamUnits::category)
    {
        addAndMakeVisible(bias);
        addAndMakeVisible(alpha);
        addAndMakeVisible(directionality);
        addAndMakeVisible(type);

        Palette::setKnobColoursOfComponent(&alpha, Palette::colours[4]);
        Palette::setKnobColoursOfComponent(&bias, Palette::colours[4]);
        Palette::setKnobColoursOfComponent(&type, Palette::colours[4]);
        Palette::setKnobColoursOfComponent(&directionality, Palette::colours[4]);

        p.treeState.addParameterListener(ParamIDs::slewType.getParamID(), this);

        makeBiasKnobTransparent();
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
        directionality.setAlpha(isDisabled ? 1.0f : 0.35f);
        directionality.setEnabled(isDisabled);
        directionality.repaint();
    }

    ParamKnob alpha;
    ParamKnob bias;
    ParamKnob directionality;
    ParamKnob type;
};