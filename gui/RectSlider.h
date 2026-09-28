#pragma once

#include "../PluginProcessor.h"
#include "juce_gui_basics/juce_gui_basics.h"

#include "GenericKnob.h"

enum RectSliderType {
    LeftJustifified,
    CenterJustifified,
    RightJustifified,
};

class RectSlider : public GenericKnob {
public:
    // fixed parameter
    RectSlider(AudioPluginAudioProcessor &p, juce::String knobName, const ParamIDs::ParameterInfo& attachmentParam, ScopeContextType scopeContextType = ScopeContextType::LR_SCOPE) 
    : GenericKnob(p, knobName, attachmentParam, scopeContextType) {
        // can't use linear bar or linear horizontal
        // cause some weird stuff happens and it immediately snaps to max. might be cause default text box flattens the pixel range?
        setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag); 
    }

    // slot macro
    RectSlider(AudioPluginAudioProcessor &p, juce::String knobName, SlotId slot, const ParamIDs::ParameterInfo& attachmentParam, ScopeContextType scopeContextType = ScopeContextType::LR_SCOPE) 
    : GenericKnob(p, knobName, slot, attachmentParam, scopeContextType) {
        setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag); 
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
        auto bounds = getLocalBounds().toFloat();

        auto modArea = bounds.removeFromBottom(modSpace());
        auto sliderBounds = bounds.removeFromBottom(sliderHeight);
        sliderBounds.reduce(15, 0);
        sliderBounds.setSize(sliderBounds.getWidth(), sliderBounds.getHeight() + dragAmount);
        sliderBounds.translate(0, -2.0f);
        float rounded = 2.0f;

        juce::Path clipTrack;
        clipTrack.addRoundedRectangle(sliderBounds, rounded);

        const auto track = sliderBounds;

        {
            juce::Graphics::ScopedSaveState clipped(g);
            g.reduceClipRegion(clipTrack);

            g.setColour(theme().sliderTrack);
            g.fillRect(sliderBounds);

            g.setColour(colours().slider.interpolatedWith(colours().sliderHeld, dragAmount));

            g.fillRect(sliderBounds.removeFromLeft(valueToProportionOfLength(getValue()) * sliderBounds.getWidth()));
        }

        // a dot where the audio really is when global drive or the like scales it, the fill staying where it's set
        if (auto scaled = scaledProportion())
        {
            const auto dotSize = track.getHeight() + 3.0f;

            g.setColour(theme().scaledMarker);
            g.fillEllipse(juce::Rectangle<float>(dotSize, dotSize).withCentre({ track.getX() + *scaled * track.getWidth(), track.getCentreY() }));
        }

        // a thin line under the bar for each modulation routed here, like the knobs' rings
        const auto x = [&] (float proportion) { return track.getX() + proportion * track.getWidth(); };
        auto y = modArea.getY() + modLineGap - 2.0f;

        for (const auto& arc : modArcs())
        {
            if (y > modArea.getBottom())
                break;

            g.setColour(theme().modulationHighlight.withMultipliedAlpha(0.4f));
            g.drawLine(x(arc.from), y, x(arc.to), y, 1.0f);

            g.setColour(arc.colour);
            g.drawLine(x(arc.base), y, x(arc.at), y, 2.0f);
            g.fillEllipse(juce::Rectangle<float>(3.0f, 3.0f).withCentre({ x(arc.at), y }));

            y += modLineGap;
        }
    }

    float visibleHeight() {
        return getLookAndFeel().getLabelFont(label).getHeight() + sliderHeight + labelGap;
    }

    void resized() override {
        auto bounds = getLocalBounds().withTrimmedBottom((int) (sliderHeight + modSpace()));
        bounds.reduce(10, 0);
        bounds.setHeight(bounds.getHeight() - (int) labelGap);
        label.setBounds(bounds);
    }
    
private:
    const float sliderHeight = 4.0f;

    // room under the bar for up to three modulation lines
    static constexpr float modLineGap = 3.0f;
    float modSpace() const { return (float) juce::jmin(3, (int) mods.size()) * modLineGap; }
    static constexpr float labelGap = 2.0f;

    RectSliderType sliderType = RectSliderType::CenterJustifified;
};