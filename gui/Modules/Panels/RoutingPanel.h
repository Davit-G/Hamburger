#pragma once

#include "../Panel.h"
#include "../../RectSlider.h"
#include "../../TextSlider.h"
#include "../../BoxToggle.h"
#include "../../../dsp/MainRouting.h"
#include "../MultibandView.h"
#include "../StageCounter.h"

class RoutingPanel : public Panel
{
public:
    explicit RoutingPanel(AudioPluginAudioProcessor& p)
        : Panel(p, "ROUTING"),
          stackFilterFreq(p, "FREQ", ParamIDs::stackFilterFreq),
          stackFilterQ(p, "Q", ParamIDs::stackFilterQ),
          stackRotation(p, "ROTATION", ParamIDs::stackRotation),
          stageCounter(p, "STAGES", ParamIDs::stackCount),
          stackGain(p, "GAIN", ParamIDs::stackGain),
          stackMix(p, "MIX", ParamIDs::stackMix),
          bandView(p)
    {
        addChildComponent(bandView);
        bandView.onBandSelected = [this](int band) { selectSlot(band); };

        for (auto* slider : allSliders())
            addChildComponent(*slider);

        for (auto* text : stackKnobs())
            addChildComponent(*text);

        // the stack's own settings, its filter's and the extras, a tab each
        for (auto* tab : { &stackTab, &filterTab, &extraTab })
        {
            addChildComponent(tab);
            tab->setClickingTogglesState(true);
            tab->setRadioGroupId(tabRadioGroup);
            tab->onClick = [this] { showControlsForRouting(); };
        }

        stackTab.setToggleState(true, juce::dontSendNotification);

        addChildComponent(stackFlip);
        stackFlip.setTooltip(ParamIDs::stackFlip.paramTooltip);
        stackFlipAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            p.treeState, ParamIDs::stackFlip.getParamID(), stackFlip);

        // by index, the parameter has more choices than there are filters, the rest kept for ones to come
        addChildComponent(stackFilterBox);
        stackFilterBox.setTooltip(ParamIDs::stackFilter.paramTooltip);
        stackFilterBox.addItemList(ParamIDs::stackFilterTypes.categories, 1);

        stackFilter = dynamic_cast<juce::AudioParameterChoice*>(
            p.treeState.getParameter(ParamIDs::stackFilter.getParamID()));
        jassert(stackFilter);

        if (stackFilter != nullptr)
        {
            stackFilterAttachment = std::make_unique<juce::ParameterAttachment>(
                *stackFilter, [this](float) { showControlsForRouting(); }, nullptr);

            stackFilterBox.onChange = [this]
            {
                if (const auto index = stackFilterBox.getSelectedItemIndex(); index >= 0)
                    stackFilterAttachment->setValueAsCompleteGesture((float) index);
            };
        }

        setActiveSlot(0);

        routing = dynamic_cast<juce::AudioParameterChoice*>(
            p.treeState.getParameter(ParamIDs::mainRouting.getParamID()));
        jassert(routing);

        if (routing != nullptr)
            routingAttachment = std::make_unique<juce::ParameterAttachment>(
                *routing, [this](float) { showControlsForRouting(); }, nullptr);

        // multiband's own band count, which the band view adds to and takes from
        bandCount = dynamic_cast<juce::AudioParameterInt*>(
            p.treeState.getParameter(ParamIDs::bandCount.getParamID()));
        jassert(bandCount);

        if (bandCount != nullptr)
            bandCountAttachment = std::make_unique<juce::ParameterAttachment>(
                *bandCount, [this](float) { showControlsForRouting(); }, nullptr);

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
    bool selectPosition(int position)
    {
        const auto layout = MainRouting::bandLayout(currentRouting(), currentBandCount());

        if (layout.numBands > 0)
            return position >= 0 && position < layout.numBands && selectSlot(layout.slots[(size_t) position]);

        return selectSlot(position);
    }

    // false for a slot the current routing leaves idle, so there's nothing to pick
    bool selectSlot(int slot)
    {
        if (! MainRouting::usesSlot(currentRouting(), currentBandCount(), slot))
            return false;

        setActiveSlot(slot);

        if (onSlotSelected != nullptr)
            onSlotSelected(slot);

        return true;
    }

    void setActiveSlot(int slot)
    {
        activeSlot = slot;
        bandView.setActiveBand(activeSlot);
    }

    // the controls take the colours of whichever distortion the box above is showing, apart from a stack's own
    void setAccentColour(AccentColours Theme::* newDistortionAccent)
    {
        distortionAccent = newDistortionAccent;
        applyAccent();
    }

    void lookAndFeelChanged() override { applyAccent(); }

    // a line between the distortion above and the stack's settings, which would otherwise read as more of it
    void paint(juce::Graphics& g) override
    {
        if (! isStack())
            return;

        constexpr auto sidePadding = boxPadding - boxInset;
        g.setColour(theme().divider);
        g.fillRect(getLocalBounds().reduced(sidePadding, 0).removeFromTop(1));
    }

    void resized() override { layOutVisible(); }

private:
    int currentRouting() const { return routing != nullptr ? routing->getIndex() : MainRouting::stack; }
    bool isStack() const { return currentRouting() == MainRouting::stack; }

    // the stack's big counter, and the gain and mix
    std::array<GenericKnob*, 3> stackKnobs() { return { &stageCounter, &stackGain, &stackMix }; }

    void applyAccent()
    {
        accent = isStack() ? &Theme::stack : distortionAccent;
        Panel::lookAndFeelChanged();

        stackFlip.setAccent(accent);

        for (auto* text : stackKnobs())
            text->setAccent(accent);

        stackFilterBox.setColour(juce::ComboBox::textColourId, (theme().*accent).main);
        stackFilterBox.setColour(juce::ComboBox::arrowColourId, (theme().*accent).main);
        repaint();
    }

    int currentBandCount() const { return bandCount != nullptr ? bandCount->get() : 2; }

    std::array<RectSlider*, 3> allSliders()
    {
        return { &stackFilterFreq, &stackFilterQ, &stackRotation };
    }

    int currentStackFilter() const { return stackFilter != nullptr ? stackFilter->getIndex() : MainRouting::noFilter; }

    void showControlsForRouting()
    {
        for (auto* slider : allSliders())
            slider->setVisible(false);

        for (auto* text : stackKnobs())
            text->setVisible(false);

        const auto current = currentRouting();

        for (auto* tab : { &stackTab, &filterTab, &extraTab })
            tab->setVisible(current == MainRouting::stack);

        stackFlip.setVisible(false);
        stackFilterBox.setVisible(false);
        bandView.setVisible(false);

        switch (current)
        {
            case MainRouting::stack:
            {
                const auto filter = currentStackFilter();
                const auto isBand = filter == MainRouting::bandpassFilter || filter == MainRouting::notchFilter;
                const auto onFilter = filterTab.getToggleState();

                for (auto* text : stackKnobs())
                    text->setVisible(stackTab.getToggleState());

                stackFlip.setVisible(extraTab.getToggleState());
                stackFilterBox.setVisible(onFilter);
                stackFilterBox.setSelectedItemIndex(filter, juce::dontSendNotification);
                stackFilterFreq.setVisible(onFilter && (filter == MainRouting::allpassFilter || isBand));
                stackFilterQ.setVisible(onFilter && isBand);
                stackRotation.setVisible(onFilter && filter == MainRouting::hilbertFilter);
                break;
            }
            
            case MainRouting::multiband:
            case MainRouting::exciter:
                bandView.setLayout(MainRouting::bandLayout(current, currentBandCount()), current == MainRouting::exciter);
                bandView.setVisible(true);
                break;

            default: break;
        }

        // a slot this routing leaves idle would be a box of controls that do nothing
        if (! MainRouting::usesSlot(current, currentBandCount(), activeSlot))
            selectSlot(0);

        layOutVisible();
        applyAccent();

        if (onRoutingChanged != nullptr)
            onRoutingChanged();
    }

    /*  The tabs centred along the top, and under them whichever's open, centred both ways in the room left: the counter
        beside the gain and mix, the filter's dropdown beside its controls, or the flip. */
    void layOutStack(juce::Rectangle<int> bounds)
    {
        auto tabs = bounds.removeFromTop(tabHeight).withSizeKeepingCentre(tabWidth * 3, tabHeight);

        for (auto* tab : { &stackTab, &filterTab, &extraTab })
            tab->setBounds(tabs.removeFromLeft(tabWidth));

        const auto rowHeight = juce::jmin(compactRowHeight, bounds.getHeight() / 2);

        if (stackTab.getToggleState())
        {
            auto block = bounds.withSizeKeepingCentre(counterWidth + columnGap + stackTextWidth, rowHeight * 2);
            stageCounter.setBounds(block.removeFromLeft(counterWidth));
            block.removeFromLeft(columnGap);
            stackGain.setBounds(block.removeFromTop(rowHeight));
            stackMix.setBounds(block);
            return;
        }

        if (extraTab.getToggleState())
        {
            stackFlip.setBounds(bounds.withSizeKeepingCentre(flipWidth, rowHeight));
            return;
        }

        std::vector<RectSlider*> filterControls;

        for (auto* slider : { &stackFilterFreq, &stackFilterQ, &stackRotation })
            if (slider->isVisible())
                filterControls.push_back(slider);

        const auto rows = (int) filterControls.size();
        auto block = bounds.withSizeKeepingCentre(filterBoxWidth + (rows > 0 ? columnGap + filterControlsWidth : 0), rowHeight * 2);

        stackFilterBox.setBounds(block.removeFromLeft(filterBoxWidth).withSizeKeepingCentre(filterBoxWidth, rowHeight - cellInset));
        placeColumn(block.removeFromRight(filterControlsWidth).withSizeKeepingCentre(filterControlsWidth, rowHeight * rows), filterControls.data(), rows,
                    RectSliderType::LeftJustifified);
    }

    void layOutVisible()
    {
        // flush with the box's sides and bottom. the box keeps the gap above it, under the levels strip
        if (bandView.isVisible())
        {
            bandView.setBounds(getLocalBounds());
            return;
        }

        // everything else keeps the panels' padding, which the module leaves out at the sides
        constexpr auto sidePadding = boxPadding - boxInset;

        auto bounds = getLocalBounds().withTrimmedLeft(sidePadding)
                                      .withTrimmedRight(sidePadding)
                                      .reduced(rowInset);

        // clear of the divider along the top
        layOutStack(bounds.withTrimmedTop(dividerClearance));
    }

    RectSlider stackFilterFreq, stackFilterQ, stackRotation;
    StageCounter stageCounter;
    TextSlider stackGain, stackMix;
    BoxToggle stackFlip { "FLIP" };

    // a tab along the top of the stack, its name lit and underlined in the stack's colour while it's open
    struct TabButton : juce::Button
    {
        explicit TabButton(const juce::String& name) : juce::Button(name) {}

        void paintButton(juce::Graphics& g, bool hovered, bool) override
        {
            const auto open = getToggleState();

            g.setColour(open ? theme().textHeader : hovered ? theme().textHeaderHover : theme().textHeaderIdle);
            g.setFont(getLookAndFeel().getPopupMenuFont().withHeight(13.0f));
            g.drawText(getButtonText(), getLocalBounds(), juce::Justification::centred, false);

            if (open)
            {
                g.setColour(theme().stack.main);
                g.fillRect(getLocalBounds().removeFromBottom(2).reduced(getWidth() / 4, 0));
            }
        }
    };

    TabButton stackTab { "STACK" }, filterTab { "FILTER" }, extraTab { "EXTRA" };
    juce::ComboBox stackFilterBox;

    juce::AudioParameterChoice* stackFilter = nullptr;
    std::unique_ptr<juce::ParameterAttachment> stackFilterAttachment;

    MultibandView bandView;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> stackFlipAttachment;

    int activeSlot = 0;
    AccentColours Theme::* distortionAccent = &Theme::plain;

    static constexpr int tabRadioGroup = 8202;
    static constexpr int tabHeight = 20, tabWidth = 60;

    juce::AudioParameterChoice* routing = nullptr;
    std::unique_ptr<juce::ParameterAttachment> routingAttachment;

    juce::AudioParameterInt* bandCount = nullptr;
    std::unique_ptr<juce::ParameterAttachment> bandCountAttachment;

    static constexpr int rowInset = 8;
    static constexpr int cellInset = 6;

    // what each tab's controls take up, about as wide as they draw so that centring them looks centred
    static constexpr int counterWidth = 125, stackTextWidth = 110, filterBoxWidth = 120, filterControlsWidth = 140, flipWidth = 60;
    static constexpr int columnGap = 24;
    static constexpr int dividerClearance = 4; // between the divider along the top and the stack's controls

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RoutingPanel)
};
