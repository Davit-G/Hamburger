#pragma once

#include "../PluginProcessor.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "KnobUtils.h"

enum RectSliderType {
    LeftJustifified,
    CenterJustifified,
    RightJustifified,
};

class RectSlider : public juce::Slider, private juce::Timer {
public:
    RectSlider(AudioPluginAudioProcessor &p, juce::String knobName, juce::ParameterID attachmentID, ParamUnits knobUnit = ParamUnits::none, ScopeContextType scopeContextType = ScopeContextType::LR_SCOPE) : processorRef(p), kName(knobName), unit(knobUnit) {
        knobAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.treeState, attachmentID.getParamID(), *this);
        jassert(knobAttachment);

        auto knobParamRange = p.treeState.getParameterRange(attachmentID.getParamID());
        
        if (knobUnit == ParamUnits::x || knobUnit == ParamUnits::category) {
            setRange(knobParamRange.start, knobParamRange.end, 1.0);
        } else {
            setRange(knobParamRange.start, knobParamRange.end, 0.001);
        }

        setPaintingIsUnclipped(true);
        setBufferedToImage(true);

        setName(knobName);

        preferredScopeContextType = scopeContextType;

        onDragStart = [this] { 
            // display the parameter value as the text instead of the parameter name
            this->isDragging = true;
            this->text.setText(createParamString(this->getValue(), this->unit), juce::dontSendNotification);
            processorRef.getScopeContext().setType(preferredScopeContextType);

            this->dragAmount = 1.0f;
            startTimerHz(60);
            repaint();
        };

        // when value is changing, set it to what the knob is, but only if we're dragging
        onValueChange = [this] {
            if (this->isDragging) {
                auto value = this->getValue();
                this->text.setText(createParamString(value, this->unit), juce::dontSendNotification);
            } else {
                this->text.setText(this->kName, juce::dontSendNotification);
            }
        };

        onDragEnd = [this] { 
            // display the parameter name as the text again
            this->isDragging = false;
            this->text.setText(this->kName, juce::dontSendNotification);
            processorRef.getScopeContext().startDecaying();
        };

        if (getParentComponent() != nullptr) { // on linux the parent happens to be broken somehow
            auto font = getParentComponent()->getLookAndFeel().getLabelFont(text);
            text.setFont(font);
        }

        startTimerHz(60);

        // can't use linear bar or linear horizontal
        // cause some weird stuff happens and it immediately snaps to max. might be cause default text box flattens the pixel range?
        setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag); 

        setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);

        auto font = getParentComponent()->getLookAndFeel().getLabelFont(text);
        text.setFont(font);

        addAndMakeVisible(text);

        text.setJustificationType(juce::Justification::centredBottom);
        text.setInterceptsMouseClicks(false, false);
        text.setText(kName, juce::dontSendNotification);
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

    ~RectSlider() override {
        stopTimer();
        knobAttachment = nullptr;
    }

    void setJustification(const RectSliderType type) {
        sliderType = type;

        switch (type)
        {
        case RectSliderType::LeftJustifified:
            text.setJustificationType(juce::Justification::bottomLeft);
            break;
        case RectSliderType::CenterJustifified:
            text.setJustificationType(juce::Justification::centredBottom);
            break;
        case RectSliderType::RightJustifified:
            text.setJustificationType(juce::Justification::bottomRight);
            break;
        default:
            break;
        }
    }

    void paint (juce::Graphics &g) override {
        // auto outline = this->findColour(juce::Slider::rotarySliderOutlineColourId);
        auto knobColour = this->findColour(juce::Slider::rotarySliderFillColourId);

        auto bounds = getLocalBounds().toFloat();

        auto sliderBounds = bounds.removeFromBottom(sliderHeight);
        sliderBounds.reduce(15, 0);
        float rounded = 2.0f;

        juce::Path clipTrack;
        clipTrack.addRoundedRectangle(sliderBounds, rounded);

        g.reduceClipRegion(clipTrack);

        auto centerBounds = getLocalBounds().toFloat().removeFromBottom(sliderHeight);
        centerBounds.reduce(15, 0);

        g.setColour(juce::Colours::darkgrey);
        g.fillRect(sliderBounds);

        g.setColour (knobColour);

        switch (sliderType)
        {
        case RectSliderType::LeftJustifified:
            g.fillRect(sliderBounds.removeFromLeft(valueToProportionOfLength(getValue()) * sliderBounds.getWidth()));
            break;
        
        case RectSliderType::CenterJustifified:
            centerBounds.reduce(static_cast<int>((1.0f - valueToProportionOfLength(getValue()) ) * 0.5f * sliderBounds.getWidth()), 0);

            g.fillRect(centerBounds);
            break;
            
        case RectSliderType::RightJustifified:
            g.fillRect(sliderBounds.removeFromRight(valueToProportionOfLength(getValue()) * sliderBounds.getWidth()));
            break;
        
        default:
            break;
        }
    }

    void resized() override {
        auto bounds = getLocalBounds().withTrimmedBottom(sliderHeight);
        bounds.reduce(10, 0);
        text.setBounds(bounds);
    }
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> knobAttachment = nullptr;
private:
    const float sliderHeight = 4.0f;

    RectSliderType sliderType = RectSliderType::CenterJustifified;

    float valueOnMouseDown = 0.0f;

    AudioPluginAudioProcessor &processorRef;
    ScopeContextType preferredScopeContextType = ScopeContextType::LR_SCOPE;

    juce::Rectangle<int> knobBounds;

    juce::String kName;
    ParamUnits unit;

    bool isDragging = false;

    float dragAmount = 0.0f;
    static constexpr float dragDecayRate = 0.88f; 
    static constexpr float dragDecayThreshold = 0.01f;

    juce::Label text;
};