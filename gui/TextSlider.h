#pragma once

#include "GenericKnob.h"

// A slider that is only its text, "NAME: value", dragged like the rect sliders.
class TextSlider : public GenericKnob
{
public:
    // fixed parameter
    TextSlider (AudioPluginAudioProcessor& p, juce::String knobName, const ParamIDs::ParameterInfo& info)
        : GenericKnob (p, knobName, info)
    {
        setUp();
    }

    // slot parameter
    TextSlider (AudioPluginAudioProcessor& p, juce::String knobName, SlotId slot, const ParamIDs::ParameterInfo& info,
                ScopeContextType scopeContextType = ScopeContextType::LR_SCOPE)
        : GenericKnob (p, knobName, slot, info, scopeContextType)
    {
        setUp();
    }

    void setJustification (juce::Justification justification) { 
        label.setJustificationType (justification);
    }

    void setFontScale (float scale) { label.getProperties().set ("fontScale", scale); }

    // text gets grayer as the value goes below 0dB, slowly takes the module colour above it 
    void setColourByGain (bool shouldColour) { colourByGain = shouldColour; updateText(); }

    void refreshText() { updateText(); }

    // just the value, where there's no room for the name in front of it
    void hideName() { showsName = false; updateText(); }

    // just the name, swapped for the value while hovered or dragged, and back a moment after the mouse has gone
    void showValueOnHover() { valueOnHover = true; updateText(); }

    void mouseEnter (const juce::MouseEvent& e) override
    {
        GenericKnob::mouseEnter (e);
        hideValueSoon.stopTimer();
        setShowingValue (true);
    }

    void mouseExit (const juce::MouseEvent& e) override
    {
        GenericKnob::mouseExit (e);

        if (! isDragging)
            hideValueSoon.startTimer (valueHoldMs);
    }

    void editorHidden (juce::Label* labelThatWasHidden, juce::TextEditor& editor) override
    {
        GenericKnob::editorHidden (labelThatWasHidden, editor);
        updateText();
    }

    // the gain colour goes over the plain text colour the base class sets
    void lookAndFeelChanged() override
    {
        GenericKnob::lookAndFeelChanged();
        updateText();
    }

    void paint (juce::Graphics&) override {}

    void resized() override { 
        label.setBounds (getLocalBounds().withTrimmedRight (showsLock() ? (int) lockSize + 2 : 0));
    }

protected:
    juce::Rectangle<float> lockArea() const override
    {
        return getLocalBounds().toFloat().removeFromRight (lockSize).withSizeKeepingCentre (lockSize, lockSize);
    }

private:
    void setUp()
    {
        setSliderStyle (juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
        label.setJustificationType (juce::Justification::centredLeft);

        onDragStart = [this] { isDragging = true; processorRef.getScopeContext().setType (preferredScopeContextType); };
        onValueChange = [this] { updateText(); };
        onDragEnd = [this]
        {
            isDragging = false;
            processorRef.getScopeContext().startDecaying();

            if (! isMouseOver())
                hideValueSoon.startTimer (valueHoldMs);
        };

        updateText();
    }

    juce::String valueText (float value) const
    {
        switch (unit)
        {
            case ParamUnits::db:      return formatDecibels (value);
            case ParamUnits::percent: return formatPercent (value * 0.01f);
            default:                  return createParamString (value, unit);
        }
    }

    void setShowingValue (bool shouldShow)
    {
        showingValue = shouldShow;
        updateText();
    }

    // modulated, the value's where the modulation has it right now, and it's all in the modulation colour
    void updateText()
    {
        const auto modulated = modulatedValue();
        const auto value = modulated.value_or ((float) getValue());

        if (valueOnHover)
            label.setText (showingValue ? valueText (value) : kName, juce::dontSendNotification);
        else
            label.setText ((showsName ? kName + ": " : juce::String()) + valueText (value), juce::dontSendNotification);

        label.setColour (juce::Label::textColourId, modulated ? theme().modulationHighlight : colourByGain ? gainColour() : colours().text);
    }

    void modulationMoved() override { updateText(); }
    void modulationChanged() override { updateText(); }

    juce::Colour gainColour() const
    {
        const auto db = (float) getValue();

        if (db < 0.0f)
            return colours().text.interpolatedWith (theme().gainTextLow, juce::jmin (1.0f, -db / dbGrayThres));

        return colours().text.interpolatedWith (colours().main, juce::jmin (1.0f, db / dbColorThres));
    }

    static constexpr float dbGrayThres = 24.0f;
    static constexpr float dbColorThres = 12.0f;

    static constexpr int valueHoldMs = 2000;

    bool colourByGain = false;
    bool showsName = true;
    bool valueOnHover = false, showingValue = false;

    juce::TimedCallback hideValueSoon { [this]
    {
        hideValueSoon.stopTimer();
        setShowingValue (false);
    } };
};
