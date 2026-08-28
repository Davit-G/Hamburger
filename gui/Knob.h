#pragma once

#include "../PluginProcessor.h"
#include "KnobUtils.h"

class ParamKnob : public juce::Component, private juce::Timer
{
public:
    ParamKnob(AudioPluginAudioProcessor &p, juce::String knobName, juce::ParameterID attachmentID, ParamUnits knobUnit = ParamUnits::none, ScopeContextType scopeContextType = ScopeContextType::LR_SCOPE): processorRef(p), kName(knobName), unit(knobUnit) {
        knobAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.treeState, attachmentID.getParamID(), knob);
        jassert(knobAttachment);

        auto knobParamRange = p.treeState.getParameterRange(attachmentID.getParamID());

        knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        knob.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        
        if (knobUnit == ParamUnits::x || knobUnit == ParamUnits::category) {
            knob.setRange(knobParamRange.start, knobParamRange.end, 1.0);
        } else {
            knob.setRange(knobParamRange.start, knobParamRange.end, 0.001);
        }

        knob.setPaintingIsUnclipped(true);

        setPaintingIsUnclipped(true);
        setBufferedToImage(true);

        setName(knobName);

        preferredScopeContextType = scopeContextType;

        knob.onDragStart = [this] { 
            // display the parameter value as the text instead of the parameter name
            this->isDragging = true;
            this->label.setText(createParamString(this->knob.getValue(), this->unit), juce::dontSendNotification);
            processorRef.getScopeContext().setType(preferredScopeContextType);

            this->dragAmount = 1.0f;
            startTimerHz(60);
            repaint();
        };

        // when value is changing, set it to what the knob is, but only if we're dragging
        knob.onValueChange = [this] {
            if (this->isDragging) {
                auto value = this->knob.getValue();
                this->label.setText(createParamString(value, this->unit), juce::dontSendNotification);
            } else {
                this->label.setText(this->kName, juce::dontSendNotification);
            }
        };

        knob.onDragEnd = [this] { 
            // display the parameter name as the text again
            this->isDragging = false;
            this->label.setText(this->kName, juce::dontSendNotification);
            processorRef.getScopeContext().startDecaying();
        };

        addAndMakeVisible(knob);

        if (getParentComponent() != nullptr) { // on linux the parent happens to be broken somehow
            auto font = getParentComponent()->getLookAndFeel().getLabelFont(label);
            label.setFont(font);

        }

        label.setColour(juce::Label::textColourId, juce::Colours::white);
        label.setJustificationType(juce::Justification::centredTop);
        addAndMakeVisible(label);

        label.setText(kName, juce::dontSendNotification);

        startTimerHz(60);
    }
	

    void paint(juce::Graphics &g) override
    {
        // auto bounds = getLocalBounds();
        // drawRotarySlider(g, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), knob.valueToProportionOfLength(knob.getValue()), 0.0f, 1.0f);
        auto outline = this->findColour(juce::Slider::rotarySliderOutlineColourId);
        auto knobColour = this->findColour(juce::Slider::rotarySliderFillColourId);

        auto bounds = juce::Rectangle<int>(knobBounds).toFloat().reduced(5.0f);

        g.setColour(outline);
        
        float size = std::min(knobBounds.getWidth(), knobBounds.getHeight());

        // the knob background
        // g.fillEllipse(juce::Rectangle<float>(size, size).reduced(5.0f).withCentre(bounds.getCentre()));

        // some circles or something
        g.setColour(knobColour);

        // g.drawEllipse(Rectangle<float>(size, size).reduced(7.0f).withCentre(bounds.getCentre()), 1.0f);
        g.drawEllipse(juce::Rectangle<float>(size, size).reduced(12.0f).withCentre(bounds.getCentre()), 2.0f);
        g.drawEllipse(juce::Rectangle<float>(size, size).reduced(20.0f).withCentre(bounds.getCentre()), 4.0f + dragAmount * 4.0f);
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

    ~ParamKnob() override {
        stopTimer();
        knobAttachment = nullptr;
    }

    void resized() override
    {
        auto amt = knob.valueToProportionOfLength(knob.getValue());

        auto bounds = getLocalBounds();
        label.setBounds(bounds.removeFromBottom(18));
        knob.setBounds(bounds);

        knobBounds = bounds;
    }

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> knobAttachment = nullptr;
    juce::Slider knob;
private:
    AudioPluginAudioProcessor &processorRef;
    ScopeContextType preferredScopeContextType = ScopeContextType::LR_SCOPE;

    juce::Label label;

    juce::Rectangle<int> knobBounds;

    juce::String kName;
    ParamUnits unit;

    bool isDragging = false;

    float dragAmount = 0.0f;
    static constexpr float dragDecayRate = 0.88f; 
    static constexpr float dragDecayThreshold = 0.01f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParamKnob)
};