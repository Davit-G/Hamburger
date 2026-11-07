#pragma once

#include "../LookAndFeel/Theme.h"

#include "../RectSlider.h"
#include "../Knob.h"

// what placeColumn trims off the top and bottom of each row
inline constexpr int columnRowInset = 2;

// stacks count sliders down a column, all sharing one justification
inline void placeColumn(juce::Rectangle<int> column, RectSlider* const* first, int count,
                        RectSliderType justification)
{
    if (count <= 0)
        return;

    const auto rowHeight = column.getHeight() / count;

    for (int i = 0; i < count; ++i)
    {
        first[i]->setJustification(justification);
        first[i]->setBounds(column.removeFromTop(rowHeight).reduced(0, columnRowInset));
    }
}

class Panel : public juce::Component
{
public:
    static constexpr int columnShare = 3;
    static constexpr int compactRowHeight = 36; // one rect slider's row wherever they sit in a compact box

    static constexpr int boxInset = 4;
    static constexpr int boxPadding = 12;
    static constexpr float boxCornerSize = 15.0f;

    Panel(AudioPluginAudioProcessor &p, juce::String theName, AccentColours Theme::* accentColours = &Theme::plain,
          SlotId slotId = SlotId{ModuleId::main, 0}) : slot(slotId), accent(accentColours), name(theName) {
        setName(theName);
    }

    AccentColours Theme::* getAccent() const { return accent; }

    // the knobs the panel holds are drawn in its colours
    void lookAndFeelChanged() override
    {
        for (auto* child : getChildren())
            if (auto* knob = dynamic_cast<GenericKnob*>(child))
                knob->setAccent(accent);
    }

    // not used in ore or post
    bool usesCompactLayout() const { return slot.module == ModuleId::main; }

    SlotId slot;

    static juce::Rectangle<int> dialAreaOf(juce::Rectangle<int> mainBounds)
    {
        return mainBounds.withTrimmedBottom(ParamKnob::labelHeight);
    }

    virtual ~Panel() = default;

protected:
    // shows the knobs, which take the panel's colours in lookAndFeelChanged
    void addKnobs(std::initializer_list<juce::Component*> knobs)
    {
        for (auto* knob : knobs)
            addAndMakeVisible(knob);
    }

    // the knobs in rows of `columns`, filling each row before starting the next. all in one row by default
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

    /*  The main knob with the icon in its middle and its sliders around it. The main box is short, so there the sliders
        stack in a column either side of the knob, the first half on the left. Everywhere else they sit under the knob:
        two either side, three with one in the middle, four with two down each side. More than four only fit in columns. */
    void layoutSaturation(juce::Component& main, juce::Component& icon, std::vector<RectSlider*> sliders)
    {
        const auto count = (int) sliders.size();

        if (usesCompactLayout() || count > 4)
        {
            auto bounds = getLocalBounds().reduced(4);

            auto left = bounds.removeFromLeft(bounds.getWidth() * columnShare / 10);
            auto right = bounds.removeFromRight(bounds.getWidth() * columnShare / (10 - columnShare));

            main.setBounds(bounds);

            const auto dial = dialAreaOf(bounds);
            const auto iconSize = juce::jmin(dial.getWidth(), dial.getHeight()) / 3;
            icon.setBounds(juce::Rectangle<int>(iconSize, iconSize).withCentre(dial.getCentre()));

            const auto leftCount = count / 2;
            const auto rows = juce::jmax(leftCount, count - leftCount);
            const auto rowHeight = juce::jmin(compactRowHeight, bounds.getHeight() / juce::jmax(rows, 1));
            const auto lift = count > 0 ? juce::roundToInt(((float) (rowHeight - columnRowInset * 2) - sliders[0]->visibleHeight()) * 0.5f) : 0;
            const auto top = icon.getBounds().getCentreY() - rows * rowHeight / 2 - juce::jmax(0, lift);

            placeColumn(left.withY(top).withHeight(leftCount * rowHeight), sliders.data(), leftCount, RectSliderType::RightJustifified);
            placeColumn(right.withY(top).withHeight((count - leftCount) * rowHeight), sliders.data() + leftCount, count - leftCount, RectSliderType::LeftJustifified);
            return;
        }

        auto bounds = getLocalBounds();

        auto knobBounds = bounds.removeFromTop((int) (bounds.getHeight() / 1.4f)).reduced(5);
        main.setBounds(knobBounds);
        knobBounds.removeFromBottom(ParamKnob::labelHeight);
        icon.setBounds(juce::Rectangle<float>(60.0f, 60.0f).withCentre(knobBounds.toFloat().getCentre()).toNearestIntEdges());

        bounds.reduce(20, 0);

        // a third of the width each, the top third of the height
        const auto width = bounds.getWidth() / 3;
        const auto left = bounds.withWidth(width).withTrimmedBottom(2 * bounds.getHeight() / 3);
        const auto right = left.withX(bounds.getRight() - width);

        auto place = [](RectSlider* slider, juce::Rectangle<int> area, RectSliderType justification) {
            slider->setJustification(justification);
            slider->setBounds(area);
        };

        if (count == 2) {
            place(sliders[0], left.translated(20, 20), RectSliderType::RightJustifified);
            place(sliders[1], right.translated(-20, 20), RectSliderType::LeftJustifified);
        } else if (count == 3) {
            place(sliders[0], left, RectSliderType::RightJustifified);
            place(sliders[1], bounds.withTrimmedLeft(width).withWidth(width).withTrimmedTop(bounds.getHeight() / 3).withTrimmedBottom(bounds.getHeight() / 3), RectSliderType::CenterJustifified);
            place(sliders[2], right, RectSliderType::LeftJustifified);
        } else {
            jassert(count == 4);
            place(sliders[0], left, RectSliderType::RightJustifified);
            place(sliders[1], left.translated(40, 40), RectSliderType::RightJustifified);
            place(sliders[2], right, RectSliderType::LeftJustifified);
            place(sliders[3], right.translated(-40, 40), RectSliderType::LeftJustifified);
        }
    }

    AccentColours Theme::* accent;

    juce::String name;
};