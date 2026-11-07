#pragma once

#include "../GenericKnob.h"

// the stack's stage count as a big number, trailed by a fading copy for each stage past the first, with arrows to step it
class StageCounter : public GenericKnob
{
public:
    StageCounter(AudioPluginAudioProcessor& p, juce::String knobName, const ParamIDs::ParameterInfo& info)
        : GenericKnob(p, knobName, info)
    {
        setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
        label.setJustificationType(juce::Justification::centredLeft);

        // the name stays up, the number is drawn rather than written into the label
        onDragStart = [this] { isDragging = true; };
        onValueChange = [this] { label.setText(kName, juce::dontSendNotification); repaint(); };
        onDragEnd = [this] { isDragging = false; };
    }

    void paint(juce::Graphics& g) override
    {
        const auto count = juce::roundToInt(getValue());
        const auto text = juce::String(count);
        const auto area = numberArea();

        g.setFont(getLookAndFeel().getLabelFont(label).withHeight(numberHeight));

        // each echo lands on top of the one before it, so the bottom right one is in front of the rest of them
        for (int echo = 1; echo < count; ++echo)
        {
            g.setColour(theme().knobText.withMultipliedAlpha(0.6f / (float) echo));
            g.drawText(text, area.translated(echoStep * (float) echo, echoStep * (float) echo), juce::Justification::centredLeft, false);
        }

        g.setColour(theme().knobText);
        g.drawText(text, area, juce::Justification::centredLeft, false);
    }

    void paintOverChildren(juce::Graphics& g) override
    {
        GenericKnob::paintOverChildren(g);

        const auto value = juce::roundToInt(getValue());

        paintArrow(g, arrowArea(+1), true, hoveredStep == +1, value < (int) getMaximum());
        paintArrow(g, arrowArea(-1), false, hoveredStep == -1, value > (int) getMinimum());
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        steppingBy = e.mods.isPopupMenu() ? 0 : stepAt(e.position);

        // one whole stage per click, as its own gesture; the attachment turns a set value into one
        if (steppingBy != 0)
        {
            setValue(getValue() + steppingBy, juce::sendNotificationSync);
            return;
        }

        GenericKnob::mouseDown(e);
    }

    // quick clicks on an arrow are just more steps, each click already took one; on the number it resets, like a knob
    void mouseDoubleClick(const juce::MouseEvent& e) override
    {
        if (stepAt(e.position) == 0)
            GenericKnob::mouseDoubleClick(e);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (steppingBy == 0)
            GenericKnob::mouseDrag(e);
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        if (std::exchange(steppingBy, 0) == 0)
            GenericKnob::mouseUp(e);
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        if (const auto step = stepAt(e.position); step != hoveredStep)
        {
            hoveredStep = step;
            repaint();
        }

        juce::Slider::mouseMove(e);
    }

    void mouseExit(const juce::MouseEvent& e) override
    {
        hoveredStep = 0;
        repaint();
        juce::Slider::mouseExit(e);
    }

    void resized() override { label.setBounds(row().withWidth((float) labelWidth).toNearestInt()); }

private:
    // the name and the number side by side, centred, the echoes trailing down from there
    juce::Rectangle<float> row() const
    {
        return getLocalBounds().toFloat().withSizeKeepingCentre((float) getWidth(), numberHeight);
    }

    juce::Rectangle<float> numberArea() const { return row().withTrimmedLeft((float) labelWidth); }

    // up to add a stage, down to take one away, stacked in a column past where the furthest echo can reach
    juce::Rectangle<float> arrowArea(int step)
    {
        const auto digitWidth = juce::GlyphArrangement::getStringWidth(getLookAndFeel().getLabelFont(label).withHeight(numberHeight), "4");

        const auto column = row().withTrimmedLeft((float) labelWidth + digitWidth + echoStep * maxEchoes + arrowGap)
                                 .withWidth(arrowSize)
                                 .withSizeKeepingCentre(arrowSize, arrowSize * 2.0f + arrowGap);

        return step > 0 ? column.withHeight(arrowSize) : column.withTrimmedTop(arrowSize + arrowGap);
    }

    // a little past each arrow still counts, they're small targets
    int stepAt(juce::Point<float> position)
    {
        if (arrowArea(+1).expanded(3.0f).contains(position)) return +1;
        if (arrowArea(-1).expanded(3.0f).contains(position)) return -1;

        return 0;
    }

    // dim at the end of the range, where there's nothing to step to
    void paintArrow(juce::Graphics& g, juce::Rectangle<float> area, bool up, bool hovered, bool canStep)
    {
        juce::Path arrow;
        const auto tri = area.reduced(1.0f, 2.0f);

        if (up)
            arrow.addTriangle(tri.getBottomLeft(), tri.getBottomRight(), { tri.getCentreX(), tri.getY() });
        else
            arrow.addTriangle(tri.getTopLeft(), tri.getTopRight(), { tri.getCentreX(), tri.getBottom() });

        g.setColour(! canStep ? theme().textDim.withMultipliedAlpha(0.3f) : hovered ? theme().knobText : theme().textDim);
        g.fillPath(arrow);
    }

    static constexpr float arrowSize = 10.0f, arrowGap = 4.0f;
    static constexpr int labelWidth = 70;
    static constexpr float numberHeight = 34.0f, echoStep = 4.0f;
    static inline const float maxEchoes = ParamIDs::stackCount.range.end - 1.0f;

    int steppingBy = 0; // the click in progress is on an arrow, not a drag
    int hoveredStep = 0;
};
