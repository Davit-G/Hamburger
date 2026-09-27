#pragma once

#include "../Panel.h"
#include "../../RectSlider.h"
#include "../../TextSlider.h"
#include "../../BoxToggle.h"
#include "../../../dsp/MainRouting.h"
#include "../MultibandView.h"
#include "../StageCounter.h"

/*  The lower part of the main view, two fifths of its box: whichever controls the current routing actually has. Every
    control is built once and only the relevant ones are shown, so switching routing is a
    visibility change rather than a rebuild. */
class RoutingPanel : public Panel
{
public:
    explicit RoutingPanel (AudioPluginAudioProcessor& p)
        : Panel (p, "ROUTING"),
          stackGain (p, "PER-STAGE GAIN", ParamIDs::stackGain),
          stackMix (p, "PER-STAGE MIX", ParamIDs::stackMix),
          stackFilterFreq (p, "FREQ", ParamIDs::stackFilterFreq),
          stackFilterQ (p, "Q", ParamIDs::stackFilterQ),
          stackRotation (p, "ROTATION", ParamIDs::stackRotation),
          msBalance (p, "M/S BALANCE", ParamIDs::msBalance),
          stackCount (p, "STAGES", ParamIDs::stackCount),
          bandView (p)
    {
        addChildComponent (bandView);
        bandView.onBandSelected = [this] (int band) { selectSlot (band); };

        for (auto* slider : allSliders())
            addChildComponent (*slider);

        addChildComponent (stackCount);
        addChildComponent (stackFlip);
        stackFlip.setTooltip (ParamIDs::stackFlip.paramTooltip);
        stackFlipAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            p.treeState, ParamIDs::stackFlip.getParamID(), stackFlip);

        // a click steps to the next filter type, the button reading the one that's on
        addChildComponent (stackFilterButton);
        stackFilterButton.setTooltip (ParamIDs::stackFilter.paramTooltip);
        stackFilterButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);

        stackFilter = dynamic_cast<juce::AudioParameterChoice*> (
            p.treeState.getParameter (ParamIDs::stackFilter.getParamID()));
        jassert (stackFilter);

        if (stackFilter != nullptr)
        {
            stackFilterAttachment = std::make_unique<juce::ParameterAttachment> (
                *stackFilter, [this] (float) { showControlsForRouting(); }, nullptr);

            stackFilterButton.onClick = [this]
            {
                stackFilterAttachment->setValueAsCompleteGesture ((float) ((stackFilter->getIndex() + 1) % ParamIDs::stackFilterTypes.categories.size()));
            };
        }

        /*  One button per main slot, for the routings with no picture of their own to click: it
            picks which slot the distortion box above is editing, and shows that slot's current
            type so the line can be read at a glance. */
        for (int i = 0; i < MainRouting::maxSlots; ++i)
        {
            auto& button = slotButtons[(size_t) i];

            addChildComponent (button);
            button.setClickingTogglesState (true);
            button.setRadioGroupId (slotRadioGroup);
            button.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
            button.setColour (juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
            button.onClick = [this, i] { selectSlot (i); };

            if (auto* type = p.treeState.getParameter (SlotId { ModuleId::main, i }.type().getParamID()))
            {
                slotTypeParams[(size_t) i] = type;
                updateSlotLabel (i);

                slotTypeAttachments[(size_t) i] = std::make_unique<juce::ParameterAttachment> (
                    *type, [this, i] (float) { updateSlotLabel (i); }, nullptr);
            }
        }

        setActiveSlot (0);

        routing = dynamic_cast<juce::AudioParameterChoice*> (
            p.treeState.getParameter (ParamIDs::mainRouting.getParamID()));
        jassert (routing);

        if (routing != nullptr)
            routingAttachment = std::make_unique<juce::ParameterAttachment> (
                *routing, [this] (float) { showControlsForRouting(); }, nullptr);

        // multiband's own band count, which the band view adds to and takes from
        bandCount = dynamic_cast<juce::AudioParameterInt*> (
            p.treeState.getParameter (ParamIDs::bandCount.getParamID()));
        jassert (bandCount);

        if (bandCount != nullptr)
            bandCountAttachment = std::make_unique<juce::ParameterAttachment> (
                *bandCount, [this] (float) { showControlsForRouting(); }, nullptr);

        showControlsForRouting();
    }

    // which main slot the distortion box above should be showing
    std::function<void (int)> onSlotSelected;

    // after the routing swaps which controls are shown
    std::function<void()> onRoutingChanged;

    // the band view runs flush to the box's edges, everything else sits inside its padding
    bool isFlush() const { return bandView.isVisible(); }

    /*  The slot shown at a position counting from the left, for the number keys: a band split runs its slots
        out of order, so the second band on screen can be slot 4. Elsewhere position and slot are the same. */
    bool selectPosition (int position)
    {
        const auto layout = MainRouting::bandLayout (currentRouting(), currentBandCount());

        if (layout.numBands > 0)
            return position >= 0 && position < layout.numBands && selectSlot (layout.slots[(size_t) position]);

        return selectSlot (position);
    }

    // false for a slot the current routing leaves idle, so there's nothing to pick
    bool selectSlot (int slot)
    {
        if (! MainRouting::usesSlot (currentRouting(), currentBandCount(), slot))
            return false;

        setActiveSlot (slot);

        if (onSlotSelected != nullptr)
            onSlotSelected (slot);

        return true;
    }

    void setActiveSlot (int slot)
    {
        activeSlot = slot;

        for (int i = 0; i < MainRouting::maxSlots; ++i)
            slotButtons[(size_t) i].setToggleState (i == activeSlot, juce::dontSendNotification);

        bandView.setActiveBand (activeSlot);
    }

    // the controls take the colours of whichever distortion the box above is showing, apart from a stack's own
    void setAccentColour (AccentColours Theme::* newDistortionAccent)
    {
        distortionAccent = newDistortionAccent;
        applyAccent();
    }

    void lookAndFeelChanged() override { applyAccent(); }

    void resized() override { layOutVisible(); }

private:
    int currentRouting() const { return routing != nullptr ? routing->getIndex() : MainRouting::stack; }

    void applyAccent()
    {
        accent = currentRouting() == MainRouting::stack ? &Theme::stack : distortionAccent;
        Panel::lookAndFeelChanged();

        stackFlip.setAccent (accent);
        stackFilterButton.setColour (juce::TextButton::textColourOffId, (theme().*accent).main);
        repaint();
    }

    int currentBandCount() const { return bandCount != nullptr ? bandCount->get() : 2; }

    // numbered with the slot's type, except in mid/side where what the slot takes matters more than what it runs
    void updateSlotLabel (int i)
    {
        if (currentRouting() == MainRouting::midSide)
        {
            slotButtons[(size_t) i].setButtonText (i == 0 ? "MID" : "SIDE");
            return;
        }

        const auto* type = dynamic_cast<juce::AudioParameterChoice*> (
            slotTypeParams[(size_t) i]);

        const auto name = type != nullptr ? type->getCurrentChoiceName() : juce::String();

        slotButtons[(size_t) i].setButtonText (juce::String (i + 1) + ": " + name);
    }

    std::array<RectSlider*, 6> allSliders()
    {
        return { &stackGain, &stackMix, &stackFilterFreq, &stackFilterQ, &stackRotation, &msBalance };
    }

    int currentStackFilter() const { return stackFilter != nullptr ? stackFilter->getIndex() : MainRouting::noFilter; }

    void showControlsForRouting()
    {
        for (auto* slider : allSliders())
            slider->setVisible (false);

        stackCount.setVisible (false);
        stackFlip.setVisible (false);
        stackFilterButton.setVisible (false);
        bandView.setVisible (false);

        const auto current = currentRouting();

        switch (current)
        {
            case MainRouting::stack:
            {
                const auto filter = currentStackFilter();
                const auto isBand = filter == MainRouting::bandpassFilter || filter == MainRouting::notchFilter;

                stackCount.setVisible (true);
                stackGain.setVisible (true);
                stackMix.setVisible (true);
                stackFlip.setVisible (true);
                stackFilterButton.setVisible (true);
                stackFilterButton.setButtonText (ParamIDs::stackFilterTypes.categories[filter]);
                stackFilterFreq.setVisible (filter == MainRouting::allpassFilter || isBand);
                stackFilterQ.setVisible (isBand);
                stackRotation.setVisible (filter == MainRouting::hilbertFilter);
                break;
            }
            
            case MainRouting::multiband:
            case MainRouting::exciter:
                bandView.setLayout (MainRouting::bandLayout (current, currentBandCount()), current == MainRouting::exciter);
                bandView.setVisible (true);
                break;

            case MainRouting::midSide:   msBalance.setVisible (true);     break;

            default: break;
        }

        // stack only ever runs one slot, and the band view is its own picker, which leaves mid/side
        const auto hasSlotButtons = current == MainRouting::midSide;

        for (int i = 0; i < MainRouting::maxSlots; ++i)
        {
            slotButtons[(size_t) i].setVisible (hasSlotButtons && MainRouting::usesSlot (current, currentBandCount(), i));
            updateSlotLabel (i);
        }

        // a slot this routing leaves idle would be a box of controls that do nothing
        if (! MainRouting::usesSlot (current, currentBandCount(), activeSlot))
            selectSlot (0);

        layOutVisible();
        applyAccent();

        if (onRoutingChanged != nullptr)
            onRoutingChanged();
    }

    void spreadAcross (juce::Rectangle<int> row, const std::vector<juce::Component*>& items)
    {
        if (items.empty() || row.getWidth() <= 0)
            return;

        const auto cellWidth = row.getWidth() / (int) items.size();

        for (auto* c : items)
            c->setBounds (row.removeFromLeft (cellWidth).reduced (cellInset));
    }

    /*  Stage count over flip on the left, gain, mix and the filter between stages stacked on the right. The filter's
        type button takes a third of its row, its one control whichever type is on the rest. */
    void layOutStack (juce::Rectangle<int> bounds)
    {
        const auto rowHeight = juce::jmin (compactRowHeight, bounds.getHeight() / 3);

        bounds = bounds.withSizeKeepingCentre (bounds.getWidth(), rowHeight * 3);

        auto left = bounds.removeFromLeft (bounds.getWidth() / 2);
        auto right = bounds;

        // two rows tall, the echoes trail down past the one the number sits in
        stackCount.setBounds (left.removeFromTop (rowHeight * 2).withTrimmedLeft (sliderInset + counterShift));

        // lined up with the start of the bars on the right
        stackFlip.setBounds (left.withTrimmedLeft (sliderInset));

        for (auto* slider : { &stackGain, &stackMix })
        {
            slider->setJustification (RectSliderType::LeftJustifified);
            slider->setBounds (right.removeFromTop (rowHeight));
        }

        stackFilterButton.setBounds (right.removeFromLeft (right.getWidth() / 3).withTrimmedLeft (sliderInset).reduced (0, cellInset / 2));

        // freq and Q share what's left of the row, otherwise the one control takes it all
        for (auto* slider : { &stackFilterFreq, &stackFilterQ, &stackRotation })
            slider->setJustification (RectSliderType::LeftJustifified);

        stackFilterFreq.setBounds (stackFilterQ.isVisible() ? right.removeFromLeft (right.getWidth() / 2) : right);
        stackFilterQ.setBounds (right);
        stackRotation.setBounds (right);
    }

    void layOutVisible()
    {
        // flush with the box's sides and bottom. the box keeps the gap above it, under the levels strip
        if (bandView.isVisible())
        {
            bandView.setBounds (getLocalBounds());
            return;
        }

        // everything else keeps the panels' padding, which the module leaves out at the sides
        constexpr auto sidePadding = boxPadding - boxInset;

        auto bounds = getLocalBounds().withTrimmedLeft (sidePadding)
                                      .withTrimmedRight (sidePadding)
                                      .reduced (rowInset);

        if (stackCount.isVisible())
        {
            layOutStack (bounds);
            return;
        }

        std::vector<juce::Component*> shown, buttons;

        for (auto* slider : allSliders())
            if (slider->isVisible())
                shown.push_back (slider);

        for (auto& button : slotButtons)
            if (button.isVisible())
                buttons.push_back (&button);

        /*  Slot buttons keep their own row along the bottom, so they don't shuffle as the
            controls above them change. */
        auto buttonRow = bounds.removeFromBottom (shown.empty() ? bounds.getHeight() : bounds.getHeight() / 2);

        spreadAcross (bounds, shown);
        spreadAcross (buttonRow, buttons);
    }

    RectSlider stackGain, stackMix, stackFilterFreq, stackFilterQ, stackRotation, msBalance;
    StageCounter stackCount;
    BoxToggle stackFlip { "FLIP PER STAGE" };
    juce::TextButton stackFilterButton;

    juce::AudioParameterChoice* stackFilter = nullptr;
    std::unique_ptr<juce::ParameterAttachment> stackFilterAttachment;

    MultibandView bandView;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> stackFlipAttachment;

    std::array<juce::TextButton, MainRouting::maxSlots> slotButtons;
    std::array<juce::RangedAudioParameter*, MainRouting::maxSlots> slotTypeParams {};
    std::array<std::unique_ptr<juce::ParameterAttachment>, MainRouting::maxSlots> slotTypeAttachments;

    int activeSlot = 0;
    AccentColours Theme::* distortionAccent = &Theme::plain;

    static constexpr int slotRadioGroup = 8201;

    juce::AudioParameterChoice* routing = nullptr;
    std::unique_ptr<juce::ParameterAttachment> routingAttachment;

    juce::AudioParameterInt* bandCount = nullptr;
    std::unique_ptr<juce::ParameterAttachment> bandCountAttachment;

    static constexpr int rowInset = 8;
    static constexpr int cellInset = 6;

    // where a rect slider's bar starts inside its bounds
    static constexpr int sliderInset = 15;

    // the stage count sits in from the left edge a little further than the sliders' bars
    static constexpr int counterShift = 16;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RoutingPanel)
};
