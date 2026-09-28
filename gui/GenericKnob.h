#pragma once

#include "../PluginProcessor.h"
#include "../utils/KnobUtils.h"
#include "LookAndFeel/Theme.h"
#include "Modules/LightButton.h"

class GenericKnob : public juce::Slider, public juce::Timer, public juce::Label::Listener, public juce::DragAndDropTarget,
                    private juce::ChangeListener, private juce::ValueTree::Listener
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

        // a start page amount scaling this moves where the audio really is, which is drawn over the knob
        macro = dynamic_cast<MacroParam*>(processorRef.treeState.getParameter(macroIdentifier.getParamID()));

        if (macro != nullptr && macro->getScaler() != nullptr)
            scalerAttachment = std::make_unique<juce::ParameterAttachment>(*macro->getScaler(), [this](float) { repaint(); }, nullptr);

        // a mod source dropped here routes to this, and whatever's routed here is drawn over it
        if (processorRef.getModMatrix().canModulate(identifier.getParamID()))
        {
            modulatable = true;
            processorRef.getModMatrix().addChangeListener(this);
            mods = processorRef.getModMatrix().connectionsTo(identifier.getParamID());
            modRefresh.startTimerHz(mods.empty() ? 0 : 30);
        }


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

        label.setJustificationType(juce::Justification::centredTop);
        label.setEditable(false, true, false);
        label.setInterceptsMouseClicks(false, false);
        label.addListener(this);
        label.setText(kName, juce::dontSendNotification);
        addAndMakeVisible(label);

        // a macro goes by whatever it's been renamed to, which a preset or the other copy of it can change
        macroIndex = ParamIDs::macroIndexOf(paramInfo);
        defaultName = kName;

        if (macroIndex >= 0)
        {
            processorRef.treeState.state.addListener(this);
            refreshMacroName();
        }

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

        editorStartText = renaming ? kName : createParamString((float) getValue(), unit);

        ed.setText(editorStartText, false);
        ed.selectAll();
    }

    void editorHidden(juce::Label *labelThatWasHidden, juce::TextEditor &ed) override {
        if (labelThatWasHidden != &label)
            return;

        auto typed = ed.getText();

        // left empty, a macro goes back to its own name
        if (std::exchange(renaming, false))
        {
            if (typed != editorStartText)
                processorRef.treeState.state.setProperty(ParamIDs::macroNameProperty(macroIndex), typed.trim(), nullptr);

            label.setText(kName, juce::dontSendNotification);
            return;
        }

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

        if (macroIndex >= 0)
            menu.addItem("Rename", [safeThis] {
                if (safeThis != nullptr)
                {
                    safeThis->renaming = true;
                    safeThis->label.showEditor();
                }
            });

        if (gainLink != nullptr)
            menu.addItem("Link IN and OUT", true, gainLink->get(), [safeThis] {
                if (safeThis != nullptr)
                    safeThis->setLinked(! safeThis->gainLink->get());
            });

        // the same as dropping a source on it. the ones already routed here are ticked
        if (modulatable)
        {
            auto& matrix = processorRef.getModMatrix();
            const auto paramID = identifier.getParamID();
            juce::PopupMenu sources;

            for (int source = 0; source < ModSources::count; ++source)
            {
                if (source == ModSources::mod1)
                    sources.addSeparator();

                const auto routed = std::any_of(mods.begin(), mods.end(), [source] (const auto& mod) { return mod.source == source; });
                const auto onItself = source == ModSources::drive && paramID == ParamIDs::globalDrive.getParamID();

                sources.addItem(matrix.sourceName(source), ! routed && ! onItself, routed, [&matrix, source, paramID] {
                    matrix.connect(source, paramID);
                });
            }

            menu.addSeparator();
            menu.addSubMenu("Assign to", sources);
        }

        // by the connections themselves, in case the list changes while the menu's open
        if (! mods.empty())
        {
            auto& matrix = processorRef.getModMatrix();
            std::vector<juce::ValueTree> connections;

            menu.addSeparator();

            for (const auto& mod : mods)
            {
                auto connection = matrix.getTree().getChild(mod.index);
                connections.push_back(connection);

                menu.addItem("Remove " + matrix.sourceName(mod.source) + " modulation", [connection] {
                    connection.getParent().removeChild(connection, nullptr);
                });
            }

            if (connections.size() > 1)
                menu.addItem("Remove all modulation", [connections] {
                    for (const auto& connection : connections)
                        connection.getParent().removeChild(connection, nullptr);
                });
        }

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
            drawGlowingGlyph(g, lockGlyph(), lockArea(), theme().lockOn);

        if (dropHover)
        {
            g.setColour(theme().modulationHighlight);
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 4.0f, 1.5f);
        }
    }

    // a mod source being dragged from the modulation box, see ModSourceButton
    static constexpr const char* modDragPrefix = "mod:";

    bool isInterestedInDragSource(const SourceDetails& details) override
    {
        return modulatable && details.description.toString().startsWith(modDragPrefix);
    }

    void itemDragEnter(const SourceDetails&) override { dropHover = true; repaint(); }
    void itemDragExit(const SourceDetails&) override { dropHover = false; repaint(); }

    void itemDropped(const SourceDetails& details) override
    {
        dropHover = false;
        repaint();
        processorRef.getModMatrix().connect(details.description.toString().fromFirstOccurrenceOf(modDragPrefix, false, false).getIntValue(),
                                            identifier.getParamID());
    }

    // the module type whose colours this is drawn in
    void setAccent(AccentColours Theme::* newAccent)
    {
        accent = newAccent;
        label.setColour(juce::Label::textColourId, colours().text);
        repaint();
    }

    // a theme change comes through here as well, and the slider only needs to rebuild itself for a new look and feel
    void lookAndFeelChanged() override
    {
        if (&getLookAndFeel() != lookAndFeelSeen)
        {
            lookAndFeelSeen = &getLookAndFeel();
            juce::Slider::lookAndFeelChanged();
        }

        setAccent(accent);
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
        processorRef.treeState.state.removeListener(this);
        processorRef.getModMatrix().removeChangeListener(this);
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

    // where the audio really is as a proportion like the knob's own, when a start page amount scales it to somewhere else
    std::optional<float> scaledProportion()
    {
        if (macro == nullptr || macro->getScaler() == nullptr || juce::approximatelyEqual(macro->getScaled(), macro->get()))
            return {};

        return (float) valueToProportionOfLength(macro->getScaled());
    }

    MacroParam* macro = nullptr;
    std::unique_ptr<juce::ParameterAttachment> scalerAttachment;

    /*  One per modulation routed here, as proportions like the knob's own: how far either way it can move it, and where
        it's moved it to from where the audio would otherwise be. */
    struct ModArc
    {
        juce::Colour colour;
        float from, to, base, at;
    };

    std::vector<ModArc> modArcs()
    {
        std::vector<ModArc> arcs;
        const auto base = scaledProportion().value_or((float) valueToProportionOfLength(getValue()));
        const auto clamp = [] (float proportion) { return juce::jlimit(0.0f, 1.0f, proportion); };

        for (const auto& mod : mods)
        {
            const auto reach = mod.pitched ? 0.0f : mod.amount;
            const auto from = mod.bipolar ? base - std::abs(reach) : base + juce::jmin(0.0f, reach);
            const auto to = mod.bipolar ? base + std::abs(reach) : base + juce::jmax(0.0f, reach);

            arcs.push_back({ theme().modSources[(size_t) mod.source], clamp(from), clamp(to), base,
                             clamp(base + processorRef.getModMatrix().shownValue(mod.index)) });
        }

        return arcs;
    }

    std::vector<ModMatrix::Shown> mods;

    AccentColours Theme::* accent = &Theme::plain;
    juce::LookAndFeel* lookAndFeelSeen = nullptr;
    const AccentColours& colours() const { return theme().*accent; }

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

    void setLinked(bool shouldLink)
    {
        gainLink->beginChangeGesture();
        *gainLink = shouldLink;
        gainLink->endChangeGesture();
    }

    float dragAmount = 0.0f;

    bool modulatable = false, dropHover = false;

    int macroIndex = -1;
    bool renaming = false;
    juce::String defaultName;

    void refreshMacroName()
    {
        const auto name = processorRef.treeState.state[ParamIDs::macroNameProperty(macroIndex)].toString();
        kName = name.isNotEmpty() ? name : defaultName;

        if (! label.isBeingEdited())
            label.setText(kName, juce::dontSendNotification);

        // anything showing the name alongside the value, like TextSlider, lays it out again
        if (onValueChange != nullptr)
            onValueChange();
    }

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override
    {
        if (tree == processorRef.treeState.state && property == ParamIDs::macroNameProperty(macroIndex))
            refreshMacroName();
    }

    void valueTreeRedirected(juce::ValueTree&) override { refreshMacroName(); }
    juce::TimedCallback modRefresh { [this] { modulationMoved(); } };

    // while modulated, every frame
    virtual void modulationMoved() { repaint(); }

    // once what's routed here, or how, has changed
    virtual void modulationChanged() {}

    // where the modulation has the parameter right now, while there is any
    std::optional<float> modulatedValue()
    {
        if (mods.empty())
            return {};

        auto proportion = scaledProportion().value_or((float) valueToProportionOfLength(getValue()));

        for (const auto& mod : mods)
            proportion += processorRef.getModMatrix().shownValue(mod.index);

        return (float) proportionOfLengthToValue(juce::jlimit(0.0f, 1.0f, proportion));
    }

    void changeListenerCallback(juce::ChangeBroadcaster*) override
    {
        mods = processorRef.getModMatrix().connectionsTo(identifier.getParamID());

        if (mods.empty())
            modRefresh.stopTimer();
        else
            modRefresh.startTimerHz(30);

        modulationChanged();
        resized();
        repaint();
    }

    static constexpr float dragDecayRate = 0.88f; 
    static constexpr float dragDecayThreshold = 0.01f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GenericKnob)
};