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
        onDragEnd = [this] { isDragging = false; processorRef.getScopeContext().startDecaying(); };

        updateText();
    }

    juce::String valueText() const
    {
        const auto value = (float) getValue();

        switch (unit)
        {
            case ParamUnits::db:      return formatDecibels (value);
            case ParamUnits::percent: return formatPercent (value * 0.01f);
            default:                  return createParamString (value, unit);
        }
    }

    void updateText()
    {
        label.setText (kName + ": " + valueText(), juce::dontSendNotification);

        if (colourByGain)
            label.setColour (juce::Label::textColourId, gainColour());
    }

    juce::Colour gainColour() const
    {
        const auto db = (float) getValue();

        if (db < 0.0f)
            return colours().text.interpolatedWith (theme().gainTextLow, juce::jmin (1.0f, -db / dbGrayThres));

        return colours().text.interpolatedWith (colours().main, juce::jmin (1.0f, db / dbColorThres));
    }

    static constexpr float dbGrayThres = 24.0f;
    static constexpr float dbColorThres = 12.0f;

    bool colourByGain = false;
};
