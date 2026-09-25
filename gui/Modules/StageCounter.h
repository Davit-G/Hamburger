#pragma once

#include "../GenericKnob.h"

class StageCounter : public GenericKnob
{
public:
    StageCounter (AudioPluginAudioProcessor& p, juce::String knobName, const ParamIDs::ParameterInfo& info)
        : GenericKnob (p, knobName, info)
    {
        setSliderStyle (juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
        label.setJustificationType (juce::Justification::centredLeft);

        // the name stays up, the number is drawn rather than written into the label
        onDragStart = [this] { isDragging = true; };
        onValueChange = [this] { label.setText (kName, juce::dontSendNotification); repaint(); };
        onDragEnd = [this] { isDragging = false; };
    }

    void paint (juce::Graphics& g) override
    {
        const auto count = juce::roundToInt (getValue());
        const auto text = juce::String (count);
        const auto font = getLookAndFeel().getLabelFont (label).withHeight (numberHeight);
        const auto area = numberArea();

        g.setFont (font);

        // each echo lands on top of the one before it, so the bottom right one is in front of the rest of them
        for (int echo = 1; echo < count; ++echo)
        {
            g.setColour (theme().stackEcho.withMultipliedAlpha (1.0f / (float) echo));
            g.drawText (text, area.translated (echoStep * (float) echo, echoStep * (float) echo), juce::Justification::centredLeft, false);
        }

        // and the number itself over all of them
        g.setColour (colours().text);
        g.drawText (text, area, juce::Justification::centredLeft, false);
    }

    void paintOverChildren (juce::Graphics& g) override
    {
        GenericKnob::paintOverChildren (g);

        const auto value = juce::roundToInt (getValue());

        paintArrow (g, arrowArea (+1), true, hoveredStep == +1, value < (int) getMaximum());
        paintArrow (g, arrowArea (-1), false, hoveredStep == -1, value > (int) getMinimum());
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        steppingBy = e.mods.isPopupMenu() ? 0 : stepAt (e.position);

        // one whole stage per click, as its own gesture; the attachment turns a set value into one
        if (steppingBy != 0)
        {
            setValue (getValue() + steppingBy, juce::sendNotificationSync);
            return;
        }

        GenericKnob::mouseDown (e);
    }

    // quick clicks on an arrow are just more steps, each click already took one; on the number it resets, like a knob
    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        if (stepAt (e.position) == 0)
            GenericKnob::mouseDoubleClick (e);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (steppingBy == 0)
            juce::Slider::mouseDrag (e);
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (steppingBy != 0)
        {
            steppingBy = 0;
            return;
        }

        juce::Slider::mouseUp (e);
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        if (const auto step = stepAt (e.position); step != hoveredStep)
        {
            hoveredStep = step;
            repaint();
        }

        juce::Slider::mouseMove (e);
    }

    void mouseExit (const juce::MouseEvent& e) override
    {
        hoveredStep = 0;
        repaint();
        juce::Slider::mouseExit (e);
    }

    void resized() override { label.setBounds (row().withWidth ((float) labelWidth).toNearestInt()); }

private:
    // the name and the number side by side, raised by half the most echoes there can be so they have room below
    juce::Rectangle<float> row() const
    {
        const auto maxEchoes = ParamIDs::stackCount.range.end - 1.0f;

        return getLocalBounds().toFloat()
                   .withSizeKeepingCentre ((float) getWidth(), numberHeight)
                   .translated (0.0f, -echoStep * maxEchoes * 0.5f);
    }

    juce::Rectangle<float> numberArea() const { return row().withTrimmedLeft ((float) labelWidth); }

    // up to add a stage, down to take one away, stacked in a column past where the furthest echo can reach
    juce::Rectangle<float> arrowArea (int step)
    {
        const auto font = getLookAndFeel().getLabelFont (label).withHeight (numberHeight);
        const auto digitWidth = juce::GlyphArrangement::getStringWidth (font, "4");
        const auto maxEchoes = ParamIDs::stackCount.range.end - 1.0f;

        const auto column = row().withTrimmedLeft ((float) labelWidth + digitWidth + echoStep * maxEchoes + arrowGap)
                                 .withWidth (arrowSize)
                                 .withSizeKeepingCentre (arrowSize, arrowSize * 2.0f + arrowGap);

        return step > 0 ? column.withHeight (arrowSize) : column.withTrimmedTop (arrowSize + arrowGap);
    }

    int stepAt (juce::Point<float> position)
    {
        // a little past each arrow still counts, they're small targets
        if (arrowArea (+1).expanded (3.0f).contains (position)) return +1;
        if (arrowArea (-1).expanded (3.0f).contains (position)) return -1;

        return 0;
    }

    // dim at the end of the range, where there's nothing to step to
    void paintArrow (juce::Graphics& g, juce::Rectangle<float> area, bool up, bool hovered, bool canStep)
    {
        juce::Path arrow;
        const auto tri = area.reduced (1.0f, 2.0f);

        if (up)
            arrow.addTriangle (tri.getBottomLeft(), tri.getBottomRight(), { tri.getCentreX(), tri.getY() });
        else
            arrow.addTriangle (tri.getTopLeft(), tri.getTopRight(), { tri.getCentreX(), tri.getBottom() });

        g.setColour (! canStep ? theme().stackArrowDisabled
                               : hovered ? theme().stackArrowHover : theme().stackArrow);
        g.fillPath (arrow);
    }

    static constexpr float arrowSize = 10.0f, arrowGap = 4.0f;

    int steppingBy = 0;  // the click in progress is on an arrow, not a drag
    int hoveredStep = 0;

    static constexpr int labelWidth = 70;
    static constexpr float numberHeight = 34.0f;
    static constexpr float echoStep = 4.0f;
};
