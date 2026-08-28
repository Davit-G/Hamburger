#pragma once

#include "../LookAndFeel/Palette.h"

#include "../RectSlider.h"

class Panel : public juce::Component
{
public:
    Panel(AudioPluginAudioProcessor &p, juce::String theName, juce::Colour color = juce::Colours::white) : name(theName) {
        setName(theName);

        Palette::setKnobColoursOfComponent(this, color);


    }

    void threeKnobLayout(juce::Component& main, juce::Component& logo, RectSlider& k1, RectSlider& k2) {
        // set these first
        k1.setJustification(RectSliderType::RightJustifified);
        k2.setJustification(RectSliderType::LeftJustifified);


        auto bounds = getLocalBounds();

        auto knobBounds = bounds.removeFromTop(bounds.getHeight() / 1.4f).reduced(5.0f);

        main.setBounds(knobBounds);
        knobBounds.removeFromBottom(18);

        float size = 60.0f;
        auto bruh = juce::Rectangle<float>(size, size).withCentre(knobBounds.toFloat().getCentre());
        
        logo.setBounds(bruh.toNearestIntEdges());

        bounds.reduce(20, 0);
        auto width = bounds.getWidth() / 3;

        k1.setBounds(bounds.removeFromLeft(width).withTrimmedBottom(2 * bounds.getHeight() / 3).translated(20, 20));
        k2.setBounds(bounds.removeFromRight(width).withTrimmedBottom(2 * bounds.getHeight() / 3).translated(-20, 20));
    }

    void fourKnobLayout(juce::Component& main, juce::Component& logo, RectSlider& k1, RectSlider& k2, RectSlider& k3)
    {   
        // set these first
        k1.setJustification(RectSliderType::RightJustifified);
        k2.setJustification(RectSliderType::CenterJustifified);
        k3.setJustification(RectSliderType::LeftJustifified);


        auto bounds = getLocalBounds();

        auto knobBounds = bounds.removeFromTop(bounds.getHeight() / 1.4f).reduced(5.0f);

        main.setBounds(knobBounds);
        knobBounds.removeFromBottom(18);

        float size = 60.0f;
        auto bruh = juce::Rectangle<float>(size, size).withCentre(knobBounds.toFloat().getCentre());
        
        logo.setBounds(bruh.toNearestIntEdges());

        bounds.reduce(20, 0);
        auto width = bounds.getWidth() / 3;

        k1.setBounds(bounds.removeFromLeft(width).withTrimmedBottom(2 * bounds.getHeight() / 3));
        k2.setBounds(bounds.removeFromLeft(width).withTrimmedTop(bounds.getHeight() / 3).withTrimmedBottom(bounds.getHeight() / 3));
        k3.setBounds(bounds.withTrimmedBottom(2 * bounds.getHeight() / 3));
    }

    void fiveKnobLayout(juce::Component& main, juce::Component& logo, RectSlider& k1, RectSlider& k2, RectSlider& k3, RectSlider& k4) {
        // set these first
        k1.setJustification(RectSliderType::RightJustifified);
        k2.setJustification(RectSliderType::RightJustifified);
        k3.setJustification(RectSliderType::LeftJustifified);
        k4.setJustification(RectSliderType::LeftJustifified);


        auto bounds = getLocalBounds();

        auto knobBounds = bounds.removeFromTop(bounds.getHeight() / 1.4f).reduced(5.0f);

        main.setBounds(knobBounds);
        knobBounds.removeFromBottom(18);

        float size = 60.0f;
        auto bruh = juce::Rectangle<float>(size, size).withCentre(knobBounds.toFloat().getCentre());
        
        logo.setBounds(bruh.toNearestIntEdges());

        bounds.reduce(20, 0);
        auto width = bounds.getWidth() / 3;

        k1.setBounds(bounds.removeFromLeft(width).withTrimmedBottom(2 * bounds.getHeight() / 3));
        k3.setBounds(bounds.removeFromRight(width).withTrimmedBottom(2 * bounds.getHeight() / 3));
        k2.setBounds(k1.getBounds().translated(40, 40));
        k4.setBounds(k3.getBounds().translated(-40, 40));
    }

    virtual ~Panel() = default;

protected:

    juce::String name;
};