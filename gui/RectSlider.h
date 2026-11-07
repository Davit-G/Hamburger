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

    void paint(juce::Graphics &g) override {
        auto bounds = getLocalBounds().toFloat();

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

            g.setColour(colours().main.interpolatedWith(colours().mainHeld, dragAmount));

            g.fillRect(sliderBounds.removeFromLeft(valueToProportionOfLength(getValue()) * sliderBounds.getWidth()));

            // each modulation routed here a strip of the bar, sharing its height: how far it can reach, and where it's moved it
            const auto arcs = modArcs();
            const auto x = [&](float proportion) { return track.getX() + proportion * track.getWidth(); };
            const auto stripHeight = track.getHeight() / (float) juce::jmax(1, (int) arcs.size());
            auto y = track.getY();

            for (const auto& arc : arcs)
            {
                g.setColour(theme().modulationHighlight.withMultipliedAlpha(0.4f));
                g.fillRect(juce::Rectangle<float>::leftTopRightBottom(x(arc.from), y, x(arc.to), y + stripHeight));

                g.setColour(arc.colour);
                g.fillRect(juce::Rectangle<float>::leftTopRightBottom(x(juce::jmin(arc.base, arc.at)), y, x(juce::jmax(arc.base, arc.at)), y + stripHeight));

                y += stripHeight;
            }
        }

        // a dot where the audio really is when global drive or the like scales it, the fill staying where it's set
        if (auto scaled = scaledProportion())
        {
            const auto dotSize = track.getHeight() + 3.0f;

            g.setColour(theme().scaledMarker);
            g.fillEllipse(juce::Rectangle<float>(dotSize, dotSize).withCentre({ track.getX() + *scaled * track.getWidth(), track.getCentreY() }));
        }
    }

    float visibleHeight() {
        return getLookAndFeel().getLabelFont(label).getHeight() + sliderHeight + labelGap;
    }

    void resized() override {
        auto bounds = getLocalBounds().withTrimmedBottom((int) sliderHeight);
        bounds.reduce(10, 0);
        bounds.setHeight(bounds.getHeight() - (int) labelGap);
        label.setBounds(bounds);
    }
    
private:
    const float sliderHeight = 4.0f;

    static constexpr float labelGap = 2.0f;

    RectSliderType sliderType = RectSliderType::CenterJustifified;
};