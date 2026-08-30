#pragma once

#include "../PluginProcessor.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "KnobUtils.h"

#include "GenericKnob.h"

enum RectSliderType {
    LeftJustifified,
    CenterJustifified,
    RightJustifified,
};

class RectSlider : public GenericKnob {
public:
    RectSlider(AudioPluginAudioProcessor &p, juce::String knobName, const ParamIDs::ParameterInfo& attachmentParam, ParamUnits knobUnit = ParamUnits::none, ScopeContextType scopeContextType = ScopeContextType::LR_SCOPE) 
    : GenericKnob(p, knobName, attachmentParam, knobUnit, scopeContextType) {
        // can't use linear bar or linear horizontal
        // cause some weird stuff happens and it immediately snaps to max. might be cause default text box flattens the pixel range?
        setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag); 
    }

    void setJustification(const RectSliderType type) {
        sliderType = type;

        switch (type)
        {
        case RectSliderType::LeftJustifified:
            label.setJustificationType(juce::Justification::bottomLeft);
            break;
        case RectSliderType::CenterJustifified:
            label.setJustificationType(juce::Justification::centredBottom);
            break;
        case RectSliderType::RightJustifified:
            label.setJustificationType(juce::Justification::bottomRight);
            break;
        default:
            break;
        }
    }

    void paint (juce::Graphics &g) override {
        // auto outline = this->findColour(juce::Slider::rotarySliderOutlineColourId);
        auto knobColour = this->findColour(juce::Slider::rotarySliderFillColourId);

        auto bounds = getLocalBounds().toFloat();

        auto sliderBounds = bounds.removeFromBottom(sliderHeight);
        sliderBounds.reduce(15, 0);
        sliderBounds.setSize(sliderBounds.getWidth(), sliderBounds.getHeight() + dragAmount);
        sliderBounds.translate(0, -2.0f);
        float rounded = 2.0f;

        juce::Path clipTrack;
        clipTrack.addRoundedRectangle(sliderBounds, rounded);

        g.reduceClipRegion(clipTrack);

        juce::Rectangle<float> centerBounds = sliderBounds;

        g.setColour(juce::Colours::darkgrey);
        g.fillRect(sliderBounds);

        g.setColour (knobColour);

        switch (sliderType)
        {
        case RectSliderType::LeftJustifified:
            g.fillRect(sliderBounds.removeFromLeft(valueToProportionOfLength(getValue()) * sliderBounds.getWidth()));
            break;
        
        case RectSliderType::CenterJustifified:
            centerBounds.reduce(static_cast<int>((1.0f - valueToProportionOfLength(getValue()) ) * 0.5f * sliderBounds.getWidth()), 0);

            g.fillRect(centerBounds);
            break;
            
        case RectSliderType::RightJustifified:
            g.fillRect(sliderBounds.removeFromRight(valueToProportionOfLength(getValue()) * sliderBounds.getWidth()));
            break;
        
        default:
            break;
        }
    }

    void resized() override {
        auto bounds = getLocalBounds().withTrimmedBottom(sliderHeight);
        bounds.reduce(10, 0);
        bounds.setHeight(bounds.getHeight() - 2.0f);
        label.setBounds(bounds);
    }
    
private:
    const float sliderHeight = 4.0f;

    RectSliderType sliderType = RectSliderType::CenterJustifified;
};