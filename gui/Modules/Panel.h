#pragma once

#include "../LookAndFeel/Palette.h"

#include "../RectSlider.h"
#include "../Knob.h"

class Panel : public juce::Component
{
public:
    Panel(AudioPluginAudioProcessor &p, juce::String theName, juce::Colour color = juce::Colours::white) : name(theName) {
        setName(theName);

        Palette::setKnobColoursOfComponent(this, color);
    }

    virtual ~Panel() = default;

protected:
    // shows the knobs, coloured in the panel's colour
    void addKnobs(std::initializer_list<juce::Component*> knobs)
    {
        for (auto* knob : knobs) {
            Palette::setKnobColoursOfComponent(knob, findColour(juce::Slider::rotarySliderFillColourId));
            addAndMakeVisible(knob);
        }
    }
    
    void layoutGrid(std::initializer_list<juce::Component*> knobs, int columns = 0)
    {
        if (columns == 0)
            columns = (int) knobs.size();

        const auto rows = ((int) knobs.size() + columns - 1) / columns;
        const auto width = (float) getWidth() / (float) columns;
        const auto height = (float) getHeight() / (float) rows;

        int i = 0;
        for (auto* knob : knobs) {
            knob->setBounds(juce::Rectangle<float>((float) (i % columns) * width, (float) (i / columns) * height, width, height).toNearestInt());
            ++i;
        }
    }

    // the main knob with the icon in its middle, and two to four sliders under it
    // two sit either side, three add one in the middle, four put two down each side
    void layoutSaturation(juce::Component& main, juce::Component& icon, std::vector<RectSlider*> sliders)
    {
        auto bounds = getLocalBounds();

        auto knobBounds = bounds.removeFromTop((int) (bounds.getHeight() / 1.4f)).reduced(5);
        main.setBounds(knobBounds);
        knobBounds.removeFromBottom(18);
        icon.setBounds(juce::Rectangle<float>(60.0f, 60.0f).withCentre(knobBounds.toFloat().getCentre()).toNearestIntEdges());

        bounds.reduce(20, 0);

        const auto width = bounds.getWidth() / 3;
        const auto left = bounds.withWidth(width).withTrimmedBottom(2 * bounds.getHeight() / 3);
        const auto right = left.withX(bounds.getRight() - width);

        auto place = [](RectSlider* slider, juce::Rectangle<int> area, RectSliderType justification) {
            slider->setJustification(justification);
            slider->setBounds(area);
        };

        if (sliders.size() == 2) {
            place(sliders[0], left.translated(20, 20), RectSliderType::RightJustifified);
            place(sliders[1], right.translated(-20, 20), RectSliderType::LeftJustifified);
        } else if (sliders.size() == 3) {
            place(sliders[0], left, RectSliderType::RightJustifified);
            place(sliders[1], bounds.withTrimmedLeft(width).withWidth(width).withTrimmedTop(bounds.getHeight() / 3).withTrimmedBottom(bounds.getHeight() / 3), RectSliderType::CenterJustifified);
            place(sliders[2], right, RectSliderType::LeftJustifified);
        } else {
            jassert(sliders.size() == 4);
            place(sliders[0], left, RectSliderType::RightJustifified);
            place(sliders[1], left.translated(40, 40), RectSliderType::RightJustifified);
            place(sliders[2], right, RectSliderType::LeftJustifified);
            place(sliders[3], right.translated(-40, 40), RectSliderType::LeftJustifified);
        }
    }

    juce::String name;
};
