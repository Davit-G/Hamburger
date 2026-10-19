#pragma once

#include "../PluginProcessor.h"
#include "LookAndFeel/HamburgerLAF.h"

// the order the modules run in, dragged into place. the clipper is pinned where it is
class FxOrderList : public juce::Component,
                    private juce::Timer
{
public:
    explicit FxOrderList(AudioPluginAudioProcessor& p) : processor(p)
    {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
        startTimerHz(10);
    }

    // each row is filled with its module's box colour
    std::function<juce::Colour(ModuleId)> colourFor;

    void paint(juce::Graphics& g) override
    {
        const auto order = dragRow >= 0 ? preview : processor.getRoutingOrder();

        auto font = juce::Font(juce::FontOptions {}).boldened();

        if(auto* laf = dynamic_cast<HamburgerLAF*>(&getLookAndFeel()))
            font = laf->getQuicksandFont();

        g.setFont(font.withHeight(juce::jmin(20.0f, rowHeight() * 0.5f)));

        for(int row = 0; row < numRows; ++row)
        {
            const auto area = rowArea(row);
            const auto id = order[(size_t) row];
            const auto colour = colourFor != nullptr ? colourFor(id) : juce::Colours::white;

            // the dragged row darkened, so it's clear which one is in hand
            g.setColour(row == dragRow ? colour.darker(0.4f) : colour);
            g.fillRoundedRectangle(area.reduced(0.0f, 1.5f), 4.0f);

            // the number on the left, the grip on the right and the name centred between them
            auto text = area.reduced(8.0f, 0.0f);
            const auto number = text.removeFromLeft(24.0f);
            const auto grip = text.removeFromRight(24.0f);

            g.setColour(juce::Colours::black.withAlpha(0.85f));
            g.drawText(juce::String(row + 1), number, juce::Justification::centred, false);

            g.setColour(juce::Colours::black);
            g.drawFittedText(names[id], text.toNearestInt(), juce::Justification::centred, 1);

            // two by three dots, left off the clipper since it can't be moved
            if(id != ModuleId::postClip)
            {
                g.setColour(juce::Colours::black.withAlpha(0.5f));

                for(int x = 0; x < 2; ++x)
                    for(int y = 0; y < 3; ++y)
                        g.fillEllipse(juce::Rectangle<float>(3.0f, 3.0f).withCentre(grip.getCentre().translated(((float) x - 0.5f) * 6.0f,(float)(y - 1) * 6.0f)));
            }
        }

        // the buns either side of the stack: a tall dome on top, and a bottom bun rounded underneath. the corners
        // facing the rows have the rows' rounding
        const auto top = juce::Rectangle<float>(0.0f, 0.0f,(float) getWidth(), topBunHeight).reduced(0.0f, 1.5f);
        const auto bottom = juce::Rectangle<float>(0.0f, rowArea(numRows - 1).getBottom(),(float) getWidth(), bottomBunHeight).reduced(0.0f, 1.5f);
        constexpr auto inner = 4.0f;

        // a rect with its top corners curved topX across and topY down, and its bottom ones bottomX by bottomY
        auto bun = [](juce::Rectangle<float> r, float topX, float topY, float bottomX, float bottomY)
        {
            juce::Path p;
            p.startNewSubPath(r.getX(), r.getY() + topY);
            p.quadraticTo(r.getX(), r.getY(), r.getX() + topX, r.getY());
            p.lineTo(r.getRight() - topX, r.getY());
            p.quadraticTo(r.getRight(), r.getY(), r.getRight(), r.getY() + topY);
            p.lineTo(r.getRight(), r.getBottom() - bottomY);
            p.quadraticTo(r.getRight(), r.getBottom(), r.getRight() - bottomX, r.getBottom());
            p.lineTo(r.getX() + bottomX, r.getBottom());
            p.quadraticTo(r.getX(), r.getBottom(), r.getX(), r.getBottom() - bottomY);
            p.closeSubPath();
            return p;
        };

        g.setColour(juce::Colour(0xffffd056));
        g.fillPath(bun(top, top.getWidth() * 0.4f, top.getHeight() - inner, inner, inner));
        g.fillPath(bun(bottom, inner, inner, 14.0f, 14.0f));
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        setMouseCursor(processor.getRoutingOrder()[(size_t) rowAt(e.position.y)] == ModuleId::postClip ? juce::MouseCursor::NormalCursor : juce::MouseCursor::PointingHandCursor);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        preview = processor.getRoutingOrder();

        const auto row = rowAt(e.position.y);

        if(preview[(size_t) row] == ModuleId::postClip)
            return;

        dragRow = row;

        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        repaint();
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if(dragRow < 0)
            return;

        auto target = rowAt(e.position.y);

        // a move only shifts the rows between where it was and where it's going, so stopping short of the clipper
        // is all it takes to leave it in place
        const auto pinned =(int) std::distance(preview.begin(), std::find(preview.begin(), preview.end(), ModuleId::postClip));
        target = dragRow < pinned ? juce::jmin(target, pinned - 1) : juce::jmax(target, pinned + 1);

        if(target == dragRow)
            return;

        // the rows between slide one place towards where it came from
        const auto begin = preview.begin();

        if(target > dragRow)
            std::rotate(begin + dragRow, begin + dragRow + 1, begin + target + 1);
        else
            std::rotate(begin + target, begin + dragRow, begin + dragRow + 1);

        dragRow = target;
        repaint();
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        if(dragRow >= 0 && preview != processor.getRoutingOrder())
            processor.setRoutingOrder(preview);

        dragRow = -1;
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
        repaint();
    }

private:
    static constexpr int numRows =(int) ModuleId::count;
    static constexpr float topBunHeight = 52.0f;
    static constexpr float bottomBunHeight = 28.0f;

    float rowHeight() const { return((float) getHeight() - topBunHeight - bottomBunHeight) /(float) numRows; }

    juce::Rectangle<float> rowArea(int row) const { return { 0.0f, topBunHeight +(float) row * rowHeight(),(float) getWidth(), rowHeight() }; }

    int rowAt(float y) const { return juce::jlimit(0, numRows - 1,(int) std::floor((y - topBunHeight) / rowHeight())); }

    // what each module is, in the order of ModuleId
    static constexpr const char* names[] { "PRE-EMPHASIS", "COMP", "NOISE", "PRE FX", "PRE", "MAIN", "POST", "POST-EMPHASIS", "CLIP" };
    static_assert(std::size(names) ==(size_t) ModuleId::count);

    // a preset or project load can change the order, and a module's type its colour
    void timerCallback() override
    {
        if(dragRow >= 0)
            return;

        juce::Array<juce::Colour> colours;

        for(int i = 0; i < numRows; ++i)
            colours.add(colourFor != nullptr ? colourFor((ModuleId) i) : juce::Colours::white);

        const auto current = processor.getRoutingOrder();

        if(current != lastOrder || colours != lastColours)
        {
            lastOrder = current;
            lastColours = colours;
            repaint();
        }
    }

    AudioPluginAudioProcessor& processor;

    UnpackedOrdering preview {}, lastOrder {};
    juce::Array<juce::Colour> lastColours;
    int dragRow = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxOrderList)
};
