#pragma once

#include "../LookAndFeel/Palette.h"

#include "../RectSlider.h"
#include "../Knob.h"

struct LayoutBounds
{
    juce::Rectangle<int> bounds;
    int width;
};

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

    Panel(AudioPluginAudioProcessor &p, juce::String theName, juce::Colour color = juce::Colours::white,
          SlotId slotId = SlotId{ModuleId::main, 0}) : slot(slotId), name(theName) {
        setName(theName);

        Palette::setKnobColoursOfComponent(this, color);
    }

    // not used in ore or post
    bool usesCompactLayout() const { return slot.module == ModuleId::main; }

    SlotId slot;

    void threeKnobLayout(juce::Component& main, juce::Component& logo, RectSlider& k1, RectSlider& k2) {
        // set these first
        k1.setJustification(RectSliderType::RightJustifified);
        k2.setJustification(RectSliderType::LeftJustifified);

        auto [bounds, width] = setupAndGetBounds(main, logo);

        k1.setBounds(bounds.removeFromLeft(width).withTrimmedBottom(2 * bounds.getHeight() / 3).translated(20, 20));
        k2.setBounds(bounds.removeFromRight(width).withTrimmedBottom(2 * bounds.getHeight() / 3).translated(-20, 20));
    }

    void fourKnobLayout(juce::Component& main, juce::Component& logo, RectSlider& k1, RectSlider& k2, RectSlider& k3)
    {   
        // set these first
        k1.setJustification(RectSliderType::RightJustifified);
        k2.setJustification(RectSliderType::CenterJustifified);
        k3.setJustification(RectSliderType::LeftJustifified);

        auto [bounds, width] = setupAndGetBounds(main, logo);

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

        auto [bounds, width] = setupAndGetBounds(main, logo);

        k1.setBounds(bounds.removeFromLeft(width).withTrimmedBottom(2 * bounds.getHeight() / 3));
        k3.setBounds(bounds.removeFromRight(width).withTrimmedBottom(2 * bounds.getHeight() / 3));
        k2.setBounds(k1.getBounds().translated(40, 40));
        k4.setBounds(k3.getBounds().translated(-40, 40));
    }

    static juce::Rectangle<int> dialAreaOf(juce::Rectangle<int> mainBounds)
    {
        return mainBounds.withTrimmedBottom(ParamKnob::labelHeight);
    }

    // used for the main distortion modes
    void compactLayout(juce::Component& main, juce::Component& logo,
                       std::initializer_list<RectSlider*> sliders, bool reserveLabel = false)
    {
        auto bounds = getLocalBounds().reduced(4);

        auto left = bounds.removeFromLeft(bounds.getWidth() * columnShare / 10);
        auto right = bounds.removeFromRight(bounds.getWidth() * columnShare / (10 - columnShare));

        main.setBounds(bounds);

        const auto dial = reserveLabel || dynamic_cast<ParamKnob*>(&main) != nullptr ? dialAreaOf(bounds) : bounds;

        const auto logoSize = juce::jmin(dial.getWidth(), dial.getHeight()) / 3;
        logo.setBounds(juce::Rectangle<int>(logoSize, logoSize).withCentre(dial.getCentre()));

        const auto count = (int) sliders.size();
        const auto leftCount = count / 2;

        const auto rows = juce::jmax(leftCount, count - leftCount);
        const auto rowHeight = juce::jmin(compactRowHeight, bounds.getHeight() / juce::jmax(rows, 1));
        const auto lift = count > 0 ? juce::roundToInt(((float) (rowHeight - columnRowInset * 2) - sliders.begin()[0]->visibleHeight()) * 0.5f) : 0;
        const auto top = logo.getBounds().getCentreY() - rows * rowHeight / 2 - juce::jmax(0, lift);

        placeColumn(left.withY(top).withHeight(leftCount * rowHeight), sliders.begin(), leftCount, RectSliderType::RightJustifified);
        placeColumn(right.withY(top).withHeight((count - leftCount) * rowHeight), sliders.begin() + leftCount, count - leftCount, RectSliderType::LeftJustifified);
    }

    LayoutBounds setupAndGetBounds(juce::Component& main, juce::Component& logo) {
        auto bounds = getLocalBounds();

        auto knobBounds = bounds.removeFromTop(bounds.getHeight() / 1.4f).reduced(5.0f);

        main.setBounds(knobBounds);
        knobBounds.removeFromBottom(ParamKnob::labelHeight);

        float size = 60.0f;
        auto bruh = juce::Rectangle<float>(size, size).withCentre(knobBounds.toFloat().getCentre());
        
        logo.setBounds(bruh.toNearestIntEdges());

        bounds.reduce(20, 0);

        return {
            bounds,
            bounds.getWidth() / 3
        };
    }

    virtual ~Panel() = default;

protected:

    juce::String name;
};