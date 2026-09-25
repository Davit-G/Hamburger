#pragma once

#include "../PluginProcessor.h"
#include "../utils/KnobUtils.h"

class GenericKnob : public juce::Slider, public juce::Timer, public juce::Label::Listener
{
public:
    //  Manual setup
    GenericKnob(AudioPluginAudioProcessor &p, juce::String knobName, const ParamIDs::ParameterInfo& attachmentInfo, ScopeContextType scopeContextType = ScopeContextType::LR_SCOPE)
    : GenericKnob(p, knobName, attachmentInfo.getParameterID(), attachmentInfo, scopeContextType) {}

    // Per-slot setup
    GenericKnob(AudioPluginAudioProcessor &p, juce::String knobName, SlotId slot, const ParamIDs::ParameterInfo& attachmentInfo, ScopeContextType scopeContextType = ScopeContextType::LR_SCOPE)
    : GenericKnob(p, knobName, paramIdFor(slot, attachmentInfo), attachmentInfo, scopeContextType) {}

private:
    GenericKnob(AudioPluginAudioProcessor &p, juce::String knobName, juce::ParameterID macroIdentifier, const ParamIDs::ParameterInfo& attachmentInfo, ScopeContextType scopeContextType)
    : identifier(macroIdentifier), processorRef(p), kName(knobName), unit(attachmentInfo.unit), paramInfo(attachmentInfo) {
        jassert (processorRef.treeState.getParameter (macroIdentifier.getParamID()) != nullptr); // wrong slot, or never registered
        knobAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processorRef.treeState, macroIdentifier.getParamID(), *this);


        setSliderStyle(juce::Slider::RotaryVerticalDrag);
        setTextBoxStyle(juce::Slider::TextBoxBelow, true, 0, 0);
         
        setPaintingIsUnclipped(true);
        setBufferedToImage(true);

        setMouseCursor(juce::MouseCursor::PointingHandCursor);
        setTooltip(paramInfo.paramTooltip);
        setName(knobName);


        preferredScopeContextType = scopeContextType;

        onDragStart = [this] { 
            // display the parameter value as the text instead of the parameter name
            this->isDragging = true;
            this->label.setText(createParamString((float) this->getValue(), this->unit), juce::dontSendNotification);
            processorRef.getScopeContext().setType(preferredScopeContextType);

            this->dragAmount = 1.0f;
            startTimerHz(60);
            repaint();
        };

        // when value is changing, set it to what the knob is, but only if we're dragging
        onValueChange = [this] {
            if (this->isDragging) {
                auto value = (float) this->getValue();
                this->label.setText(createParamString(value, this->unit), juce::dontSendNotification);
            } else {
                this->label.setText(this->kName, juce::dontSendNotification);
            }
        };

        onDragEnd = [this] { 
            // display the parameter name as the text again
            this->isDragging = false;
            this->label.setText(this->kName, juce::dontSendNotification);
            processorRef.getScopeContext().startDecaying();
        };

        if (getParentComponent() != nullptr) { // on linux the parent happens to be broken somehow
            auto font = getParentComponent()->getLookAndFeel().getLabelFont(label);
            label.setFont(font);
        }

        label.setColour(juce::Label::textColourId, juce::Colours::white);
        label.setJustificationType(juce::Justification::centredTop);
        label.setEditable(false, true, false);
        label.setInterceptsMouseClicks(false, false);
        label.addListener(this);
        label.setText(kName, juce::dontSendNotification);
        addAndMakeVisible(label);

        startTimerHz(60);
    }

public:
    void mouseDoubleClick(const juce::MouseEvent &) override {
        resetToDefault();
    }

    void mouseDown(const juce::MouseEvent & e) override {
        if (showsLock() && lockArea().contains(e.position)) {
            setLinked(false);
            return;
        }

        // text edit
        if (e.mods.isPopupMenu()) {
            showResetMenu();
            return;
        }

        // reset to default
        if (e.mods.isCommandDown()) {
            resetToDefault();
            return;
        }

        juce::Slider::mouseDown(e);
    }

    void editorShown(juce::Label *labelThatWasShown, juce::TextEditor &ed) override {
        if (labelThatWasShown != &label)
            return;

        editorStartText = createParamString((float) getValue(), unit);

        ed.setText(editorStartText, false);
        ed.selectAll();
    }

    void editorHidden(juce::Label *labelThatWasHidden, juce::TextEditor &ed) override {
        if (labelThatWasHidden != &label)
            return;

        auto typed = ed.getText();

        const bool cancelled = typed == editorStartText || typed == kName;

        if (!cancelled)
            if (auto value = parseParamString(typed, unit))
                setValue(*value, juce::sendNotificationSync);

        label.setText(kName, juce::dontSendNotification);
    }
    
    void labelTextChanged(juce::Label *) override {}

    void resetToDefault() {
        auto *param = processorRef.treeState.getParameter(identifier.getParamID());

        if (param == nullptr)
            return;
        
        param->beginChangeGesture();
        param->setValueNotifyingHost(paramInfo.range.convertTo0to1(paramInfo.defaultValue));
        param->endChangeGesture();
    }

    void showResetMenu() {
        const auto itemText = "Reset to default";

        juce::Component::SafePointer<GenericKnob> safeThis(this);

        juce::PopupMenu menu;
        menu.setLookAndFeel(&getLookAndFeel());
        menu.addItem(itemText, [safeThis] {
            if (safeThis != nullptr)
                safeThis->resetToDefault();
        });

        menu.addItem("Enter Value", [safeThis] {
            if (safeThis != nullptr)
                safeThis->label.showEditor();
        });

        if (gainLink != nullptr)
            menu.addItem("Link IN and OUT", true, gainLink->get(), [safeThis] {
                if (safeThis != nullptr)
                    safeThis->setLinked(! safeThis->gainLink->get());
            });

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withMousePosition());
    }

    //  Pairs this with the other side of its gain stage through their link parameter
    void setGainLink(juce::RangedAudioParameter* link, bool isOutputSide)
    {
        gainLink = dynamic_cast<juce::AudioParameterBool*>(link);
        jassert(gainLink != nullptr);

        linkOutputSide = isOutputSide;

        if (gainLink != nullptr)
            linkAttachment = std::make_unique<juce::ParameterAttachment>(*gainLink, [this](float) {
                resized();
                repaint();
            }, nullptr);
    }

    void paintOverChildren(juce::Graphics &g) override
    {
        if (showsLock())
            g.drawImage(lockImage, lockArea(), juce::RectanglePlacement::centred);
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

public:
    ~GenericKnob() {
        stopTimer();
        knobAttachment = nullptr;
        label.removeListener(this);
    }

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> knobAttachment = nullptr;
protected:
    AudioPluginAudioProcessor &processorRef;
    ScopeContextType preferredScopeContextType = ScopeContextType::LR_SCOPE;

    juce::ParameterID identifier;

    juce::Label label;

    juce::Rectangle<int> knobBounds;
    juce::String editorStartText;

    juce::String kName;
    ParamUnits unit;

    const ParamIDs::ParameterInfo& paramInfo;

    bool showsLock() const { return gainLink != nullptr && linkOutputSide && gainLink->get(); }

    // where the lock sits on a linked output, the top right corner unless the control says otherwise
    virtual juce::Rectangle<float> lockArea() const
    {
        return getLocalBounds().toFloat().removeFromTop(lockSize).removeFromRight(lockSize);
    }

    static constexpr float lockSize = 12.0f;


    bool isDragging = false;

    juce::AudioParameterBool* gainLink = nullptr;
    bool linkOutputSide = false;
    std::unique_ptr<juce::ParameterAttachment> linkAttachment;
    juce::Image lockImage = juce::ImageCache::getFromMemory(BinaryData::lockon_png, BinaryData::lockon_pngSize);

    void setLinked(bool shouldLink)
    {
        gainLink->beginChangeGesture();
        *gainLink = shouldLink;
        gainLink->endChangeGesture();
    }

    float dragAmount = 0.0f;
    static constexpr float dragDecayRate = 0.88f; 
    static constexpr float dragDecayThreshold = 0.01f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GenericKnob)
};