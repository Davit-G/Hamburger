#pragma once

#include "../PluginProcessor.h"
#include "LookAndFeel/HamburgerLAF.h"

// the order the modules run in, dragged into place or moved a row at a time by the arrows on the hovered row. the clipper is pinned where it is
class FxOrderList : public juce::Component,
                    private juce::Timer
{
public:
    explicit FxOrderList(AudioPluginAudioProcessor& p) : processor(p)
    {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
        startTimerHz(10);
    }

    // each row's name is in its module's box colour
    std::function<juce::Colour(ModuleId)> colourFor;

    void paint(juce::Graphics& g) override
    {
        const auto order = dragRow >= 0 ? preview : processor.getRoutingOrder();

        // the settings labels' font
        auto font = juce::Font(juce::FontOptions {});

        if (auto* laf = dynamic_cast<HamburgerLAF*>(&getLookAndFeel()))
            font = *laf->questrialFont14;

        g.setFont(font.withHeight(juce::jmin(15.0f, rowHeight() * 0.5f)));

        for (int row = 0; row < numRows; ++row)
        {
            const auto area = rowArea(row);
            const auto id = order[(size_t) row];
            const auto colour = colourFor != nullptr ? colourFor(id) : theme().plain.main;

            // the dragged row lighter, so it's clear which one is in hand
            g.setColour(row == dragRow ? theme().rowDragged : theme().settingsButton);
            g.fillRoundedRectangle(area.reduced(0.0f, 1.5f), 4.0f);

            // the arrows on the left while it's hovered, the grip on the right and the name centred between them
            auto text = area.reduced(8.0f, 0.0f);
            const auto arrows = text.removeFromLeft(arrowWidth);
            const auto grip = text.removeFromRight(24.0f);

            g.setColour(colour);
            g.drawFittedText(nameFor(id), text.toNearestInt(), juce::Justification::centred, 1);

            if (id == ModuleId::postClip)
                continue;

            // the way it can't go faint, and the one under the mouse brighter than the other
            if (row == hoverRow && dragRow < 0)
            {
                for (const auto step : { -1, 1 })
                {
                    const auto half = step < 0 ? arrows.withTrimmedBottom(arrows.getHeight() * 0.5f) : arrows.withTrimmedTop(arrows.getHeight() * 0.5f);
                    const auto centre = half.getCentre();
                    const auto alpha = ! canMove(order, row, step) ? 0.2f : step == hoverArrow ? 1.0f : 0.55f;

                    juce::Path arrow;
                    arrow.addTriangle(centre.x - 5.0f, centre.y + 2.5f * (float) -step, centre.x + 5.0f, centre.y + 2.5f * (float) -step,
                                      centre.x, centre.y - 2.5f * (float) -step);

                    g.setColour(colour.withMultipliedAlpha(alpha));
                    g.fillPath(arrow);
                }
            }

            // two by three dots, left off the clipper since it can't be moved
            g.setColour(colour.withMultipliedAlpha(0.5f));

            for (int x = 0; x < 2; ++x)
                for (int y = 0; y < 3; ++y)
                    g.fillEllipse(juce::Rectangle<float>(3.0f, 3.0f).withCentre(grip.getCentre().translated(((float) x - 0.5f) * 6.0f, (float) (y - 1) * 6.0f)));
        }
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        const auto row = rowAt(e.position.y);
        const auto arrow = arrowAt(e.position);

        setMouseCursor(processor.getRoutingOrder()[(size_t) row] == ModuleId::postClip && arrow == 0
                           ? juce::MouseCursor::NormalCursor : juce::MouseCursor::PointingHandCursor);

        if (row != hoverRow || arrow != hoverArrow)
        {
            hoverRow = row;
            hoverArrow = arrow;
            repaint();
        }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        hoverRow = -1;
        hoverArrow = 0;
        repaint();
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        preview = processor.getRoutingOrder();

        const auto row = rowAt(e.position.y);

        // an arrow swaps it with the row next to it, rather than picking it up
        if (const auto step = arrowAt(e.position); step != 0)
        {
            std::swap(preview[(size_t) row], preview[(size_t) (row + step)]);
            processor.setRoutingOrder(preview);
            hoverArrow = arrowAt(e.position);
            repaint();
            return;
        }

        if (preview[(size_t) row] == ModuleId::postClip)
            return;

        dragRow = row;

        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        repaint();
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (dragRow < 0)
            return;

        auto target = rowAt(e.position.y);

        // a move only shifts the rows between where it was and where it's going, so stopping short of the clipper
        // is all it takes to leave it in place
        const auto pinned = (int) std::distance(preview.begin(), std::find(preview.begin(), preview.end(), ModuleId::postClip));
        target = dragRow < pinned ? juce::jmin(target, pinned - 1) : juce::jmax(target, pinned + 1);

        if (target == dragRow)
            return;

        // the rows between slide one place towards where it came from
        const auto begin = preview.begin();

        if (target > dragRow)
            std::rotate(begin + dragRow, begin + dragRow + 1, begin + target + 1);
        else
            std::rotate(begin + target, begin + dragRow, begin + dragRow + 1);

        dragRow = target;
        repaint();
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        if (dragRow >= 0 && preview != processor.getRoutingOrder())
            processor.setRoutingOrder(preview);

        dragRow = -1;
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
        repaint();
    }

private:
    static constexpr int numRows = (int) ModuleId::count;
    static constexpr float arrowWidth = 24.0f;

    float rowHeight() const { return (float) getHeight() / (float) numRows; }

    juce::Rectangle<float> rowArea(int row) const { return { 0.0f, (float) row * rowHeight(), (float) getWidth(), rowHeight() }; }

    int rowAt(float y) const { return juce::jlimit(0, numRows - 1, (int) std::floor(y / rowHeight())); }

    // a module can step to the next row either way, as long as neither it nor what it would swap with is the clipper
    static bool canMove(const RoutingOrder& order, int row, int step)
    {
        const auto to = row + step;
        return juce::isPositiveAndBelow(to, numRows) && order[(size_t) row] != ModuleId::postClip && order[(size_t) to] != ModuleId::postClip;
    }

    // the arrow under a point, -1 up and 1 down, 0 off the arrows or the way it can't go
    int arrowAt(juce::Point<float> position) const
    {
        const auto row = rowAt(position.y);
        const auto arrows = rowArea(row).reduced(8.0f, 0.0f).removeFromLeft(arrowWidth);

        if (! arrows.contains(position))
            return 0;

        const auto step = position.y < arrows.getCentreY() ? -1 : 1;
        return canMove(processor.getRoutingOrder(), row, step) ? step : 0;
    }

    // what each module is running right now: main by its routing, the rest by their type where they have one
    juce::String nameFor(ModuleId id) const
    {
        auto choice = [this](const juce::String& paramId) { return dynamic_cast<juce::AudioParameterChoice*>(processor.treeState.getParameter(paramId)); };

        // as the box's type menu reads
        auto* type = choice(SlotId { id, 0 }.type().getParamID());
        const auto typeName = type != nullptr ? type->getCurrentChoiceName() : juce::String();

        switch (id)
        {
            case ModuleId::preEmphasis:    return "PRE-EMPHASIS";
            case ModuleId::dynamics:       return typeName + " COMP";
            case ModuleId::module1:        return typeName + " NOISE";
            case ModuleId::module2:        return typeName;
            case ModuleId::preDistortion:  return "PRE " + typeName;
            case ModuleId::main:
            {
                auto* routing = choice(ParamIDs::mainRouting.getParamID());
                return routing != nullptr ? routing->getCurrentChoiceName() : "MAIN";
            }
            case ModuleId::postDistortion: return "POST " + typeName;
            case ModuleId::postEmphasis:   return "POST-EMPHASIS";
            case ModuleId::postClip:       return typeName.isNotEmpty() ? typeName : "CLIP";
            case ModuleId::count:          break;
        }

        return {};
    }

    // a preset, a project load or a module's own type or routing can change what's listed, and its colour
    void timerCallback() override
    {
        if (dragRow >= 0)
            return;

        juce::StringArray names;
        juce::Array<juce::Colour> colours;

        for (int i = 0; i < numRows; ++i)
        {
            names.add(nameFor((ModuleId) i));
            colours.add(colourFor != nullptr ? colourFor((ModuleId) i) : theme().plain.main);
        }

        const auto current = processor.getRoutingOrder();

        if (current != lastOrder || names != lastNames || colours != lastColours)
        {
            lastOrder = current;
            lastNames = names;
            lastColours = colours;
            repaint();
        }
    }

    AudioPluginAudioProcessor& processor;

    RoutingOrder preview {}, lastOrder {};
    juce::StringArray lastNames;
    juce::Array<juce::Colour> lastColours;
    int dragRow = -1, hoverRow = -1, hoverArrow = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxOrderList)
};
