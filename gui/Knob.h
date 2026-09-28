#pragma once

#include "../PluginProcessor.h"
#include "../utils/KnobUtils.h"
#include "GenericKnob.h"

class ParamKnob : public GenericKnob
{
public:
    static constexpr int labelHeight = 18;

    // fixed parameter
    ParamKnob(AudioPluginAudioProcessor &p, juce::String knobName, const ParamIDs::ParameterInfo& attachmentInfo, ScopeContextType scopeContextType = ScopeContextType::LR_SCOPE) : 
    GenericKnob(p, knobName, attachmentInfo, scopeContextType) {
        modulationChanged();
    }

    // slot macro
    ParamKnob(AudioPluginAudioProcessor &p, juce::String knobName, SlotId slot, const ParamIDs::ParameterInfo& attachmentInfo, ScopeContextType scopeContextType = ScopeContextType::LR_SCOPE) : 
    GenericKnob(p, knobName, slot, attachmentInfo, scopeContextType) {
        modulationChanged();
    }

    // thickness as a share of the knob's radius
    void drawThumb(juce::Graphics &g, juce::Rectangle<float> bounds, float sliderPos, juce::Colour colour, float thickness)
    {
        bounds = bounds.reduced(5.0f);

        auto rotary = getRotaryParameters();
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotary.startAngleRadians + sliderPos * (rotary.endAngleRadians - rotary.startAngleRadians);
        auto lineW = juce::jmin(8.0f, radius * 0.5f);
        auto arcRadius = radius - lineW * 0.5f;

        juce::Line<float> marker;

        float xOffset = std::sin(toAngle) * arcRadius;
        float yOffset = -std::cos(toAngle) * arcRadius;

        marker.setStart(xOffset * 0.8f + bounds.getCentreX(), yOffset * 0.8f + bounds.getCentreY());
        marker.setEnd(xOffset + bounds.getCentreX(), yOffset + bounds.getCentreY());

        juce::Path p;
        p.addLineSegment(marker, radius * thickness);
        g.setColour(colour);
        g.strokePath(p, juce::PathStrokeType(radius * thickness, juce::PathStrokeType::JointStyle::curved, juce::PathStrokeType::EndCapStyle::rounded));
    }

    void paint(juce::Graphics &g) override
    {
        auto bounds = knobBounds.reduced(5.0f);

        float size = std::min(knobBounds.getWidth(), knobBounds.getHeight());

        // the knob background
        // g.fillEllipse(juce::Rectangle<float>(size, size).reduced(5.0f).withCentre(bounds.getCentre()));

        // some circles or something
        g.setColour(colours().main.interpolatedWith(colours().mainHeld, dragAmount));

        // g.drawEllipse(Rectangle<float>(size, size).reduced(7.0f).withCentre(bounds.getCentre()), 1.0f);
        g.drawEllipse(juce::Rectangle<float>(size, size).reduced(12.0f).withCentre(bounds.getCentre().toFloat()), 2.0f);
        g.drawEllipse(juce::Rectangle<float>(size, size).reduced(20.0f).withCentre(bounds.getCentre().toFloat()), 4.0f + dragAmount * 4.0f);

        bounds.expand(5.0f, 5.0f);
        drawThumb(g, bounds.toFloat(), (float) valueToProportionOfLength(getValue()), theme().knobThumb, 0.08f);

        // a thinner one where the audio really is, when global drive or the like scales it
        if (auto scaled = scaledProportion())
            drawThumb(g, bounds.toFloat(), *scaled, theme().scaledMarker, 0.04f);

        drawModulation(g, size, bounds.getCentre().toFloat());
    }

    /*  A thin ring inside the knob's own for each modulation routed here: how far it can reach in the modulation colour,
        and in its source's colour, a filled arc and a dot from where the knob is to where it's being moved right now */
    void drawModulation(juce::Graphics& g, float size, juce::Point<float> centre)
    {
        const auto rotary = getRotaryParameters();
        const auto angle = [&] (float proportion) { return rotary.startAngleRadians + proportion * (rotary.endAngleRadians - rotary.startAngleRadians); };

        auto radius = size * 0.5f - 28.0f;

        for (const auto& arc : modArcs())
        {
            if (radius < size * 0.2f)
                break;

            juce::Path reach, moved;
            reach.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, angle(arc.from), angle(arc.to), true);
            moved.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, angle(arc.base), angle(arc.at), true);

            g.setColour(theme().modulationHighlight.withMultipliedAlpha(0.4f));
            g.strokePath(reach, juce::PathStrokeType(1.5f));

            g.setColour(arc.colour);
            g.strokePath(moved, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.fillEllipse(juce::Rectangle<float>(4.0f, 4.0f).withCentre(centre.getPointOnCircumference(radius, angle(arc.at))));

            radius -= 4.0f;
        }
    }

    void timerCallback() override
    {
        if (isDragging) // hold at full brightness until the user lets go
            return;

        dragAmount *= dragDecayRate;

        if (dragAmount < dragDecayThreshold) {
            dragAmount = 0.0f;
            stopTimer();
        }

        repaint();
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        label.setBounds(bounds.removeFromBottom(labelHeight));

        knobBounds = bounds;

        // down from the top right corner of the knob's square, where its rings leave room
        const auto size = juce::jmin(knobBounds.getWidth(), knobBounds.getHeight());
        const auto corner = knobBounds.withSizeKeepingCentre(size, size);
        auto x = juce::jmin(corner.getRight() - dialSize / 2, getWidth() - dialSize);

        for (size_t i = 0; i < dials.size(); ++i)
            dials[i]->setBounds(x, corner.getY() + (int) i * (dialSize + 2), dialSize, dialSize);
    }

private:
    // a small dial for how far one modulation routed here reaches, in its source's colour, 0 at the top
    struct AmountDial : juce::Slider
    {
        AmountDial(juce::ValueTree c, int sourceIndex, const juce::String& sourceName) : connection(c), source(sourceIndex)
        {
            setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
            setRange(-1.0, 1.0);
            setDoubleClickReturnValue(true, 0.0);
            setMouseDragSensitivity(150);
            setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
            getValueObject().referTo(connection.getPropertyAsValue(ModMatrix::amountId, nullptr));
            setTooltip(sourceName + " amount. Double click for none");
        }

        void paint(juce::Graphics& g) override
        {
            const auto bounds = getLocalBounds().toFloat().reduced(1.5f);
            const auto radius = bounds.getWidth() * 0.5f;
            const auto centre = bounds.getCentre();
            constexpr auto reach = juce::MathConstants<float>::pi * 0.75f;

            g.setColour(theme().sliderTrack);
            g.drawEllipse(bounds, 1.5f);

            juce::Path amount;
            amount.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, 0.0f, (float) getValue() * reach, true);

            g.setColour(theme().modSources[(size_t) source]);
            g.strokePath(amount, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.fillEllipse(juce::Rectangle<float>(3.0f, 3.0f).withCentre(centre));
        }

        juce::ValueTree connection;
        int source;
    };

    // made again only when connections came or went, so dragging one doesn't pull it out from under the mouse
    void modulationChanged() override
    {
        auto& matrix = processorRef.getModMatrix();
        auto same = dials.size() == mods.size();

        for (size_t i = 0; same && i < mods.size(); ++i)
            same = dials[i]->connection == matrix.getTree().getChild(mods[i].index);

        if (same)
            return;

        dials.clear();

        for (const auto& mod : mods)
        {
            dials.push_back(std::make_unique<AmountDial>(matrix.getTree().getChild(mod.index), mod.source, matrix.sourceName(mod.source)));
            addAndMakeVisible(*dials.back());
        }

        resized();
    }

    static constexpr int dialSize = 14;
    std::vector<std::unique_ptr<AmountDial>> dials;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParamKnob)
};
