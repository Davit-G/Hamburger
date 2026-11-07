#pragma once

#include "../PluginProcessor.h"
#include "TextSlider.h"
#include "Modules/ClipIndicator.h"

// along the bottom of the whole plugin: the clipper at the end of the chain, then hamburger's power and overall gains
class PluginFooter : public juce::Component
{
public:
    static constexpr int pillInset = 4;
    static constexpr int totalHeight = 36;

    explicit PluginFooter(AudioPluginAudioProcessor& p)
        : in(p, "IN", ParamIDs::inputGain),
          wet(p, "WET", ParamIDs::mix),
          out(p, "OUT", ParamIDs::outputGain),
          clipGain(p, "GAIN", clipSlot, ParamIDs::postClipGain, ScopeContextType::CLIPPER),
          clipKnee(p, "KNEE", clipSlot, ParamIDs::postClipKnee, ScopeContextType::CLIPPER),
          clipTime(p, "TIME", clipSlot, ParamIDs::postClipTime, ScopeContextType::CLIPPER),
          clipFoldFreq(p, "HP", clipSlot, ParamIDs::postClipFoldFreq, ScopeContextType::CLIPPER),
          clipDot(p.getScopeDataCollector(), p),
          powerAttachment(p.treeState, ParamIDs::hamburgerEnabled.getParamID(), power),
          clipPowerAttachment(p.treeState, clipSlot.enabled().getParamID(), clipPower)
    {
        power.setTooltip("Enable / disable hamburger.");
        clipPower.setTooltip("Enable / disable the clipper at the end of the chain.");

        auto* link = p.treeState.getParameter(ParamIDs::gainLink.getParamID());
        in.setGainLink(link, false, p.treeState.getParameter(ParamIDs::outputGain.getParamID()));
        out.setGainLink(link, true, p.treeState.getParameter(ParamIDs::inputGain.getParamID()));

        in.setColourByGain(true);
        wet.setColourByGain(true);
        out.setColourByGain(true);
        clipGain.setColourByGain(true);

        for (auto* slider : { &clipGain, &clipKnee, &clipTime, &clipFoldFreq })
        {
            slider->showValueOnHover();
            slider->setJustification(juce::Justification::centred);
        }

        wet.setJustification(juce::Justification::centred);
        out.setJustification(juce::Justification::centredRight);

        for (auto* control : std::initializer_list<GenericKnob*> { &in, &wet, &out, &clipGain, &clipKnee, &clipTime, &clipFoldFreq })
        {
            control->setAccent(&Theme::footer);
            addChildComponent(control);
        }

        for (auto* control : std::initializer_list<GenericKnob*> { &in, &wet, &out, &clipGain })
            control->setVisible(true);

        clipType.addItemList(ParamIDs::clipTypes.categories, 1);
        clipType.setTooltip("Keeps the output under 0dB: soft clip, limiter or fold back");

        /*  By index, not a ComboBoxAttachment: that spreads the parameter's 0 to 1 over the menu's items, and the menu has
            fewer items than the parameter has choices, so the limiter was landing on the last reserved choice instead */
        clipTypeAttachment = std::make_unique<juce::ParameterAttachment>(*p.treeState.getParameter(clipSlot.type().getParamID()), [this](float index)
        {
            clipType.setSelectedItemIndex((int) index, juce::dontSendNotification);
            showClipControls();
        });

        clipType.onChange = [this]
        {
            clipTypeAttachment->setValueAsCompleteGesture((float) clipType.getSelectedItemIndex());
            showClipControls();
        };

        clipTypeAttachment->sendInitialUpdate();

        for (auto* child : std::initializer_list<juce::Component*> { &power, &clipPower, &clipType, &clipDot })
            addAndMakeVisible(child);
    }

    // the start page is narrower, so the clip type is only its arrow there
    void setCompact(bool shouldBeCompact)
    {
        compact = shouldBeCompact;
        resized();
    }

    void lookAndFeelChanged() override
    {
        clipType.setColour(juce::ComboBox::textColourId, theme().categoryText);
        clipType.setColour(juce::ComboBox::arrowColourId, theme().categoryText);
    }

    void paint(juce::Graphics& g) override
    {
        const auto bar = getBar().toFloat();

        g.setColour(theme().box);
        g.fillRect(bar);

        g.setColour(theme().headerDivider);
        g.drawLine((float) dividerX, bar.getY() + dividerInset, (float) dividerX, bar.getBottom() - dividerInset, 1.0f);
    }

    void resized() override
    {
        auto row = getBar().reduced(pillInset + edgePad, 0);

        auto clip = row.removeFromLeft(row.getWidth() / 2);
        dividerX = clip.getRight();

        clipPower.setBounds(clip.removeFromLeft(powerSize).withSizeKeepingCentre(powerSize, powerSize));
        clip.removeFromLeft(gap);
        // as far from the divider as the power button is on its other side
        clip.removeFromRight(edgePad);
        clipDot.setBounds(clip.removeFromRight(dotSize).withSizeKeepingCentre(dotSize, dotSize));
        clip.removeFromRight(gap * 2);

        // the type menu only as wide as its names, or only its arrow without room for them
        clipType.setBounds(clip.removeFromLeft(compact || clip.getWidth() - clipReadoutsWidth < clipTypeWidth ? arrowOnlyWidth : clipTypeWidth));

        row.removeFromLeft(edgePad);

        power.setBounds(row.removeFromLeft(powerSize).withSizeKeepingCentre(powerSize, powerSize));

        // a little more room around the gains, so they don't crowd the power button or the edge
        row = row.reduced(gainsPadding, 0);

        const auto third = row.getWidth() / 3;
        in.setBounds(row.removeFromLeft(third));
        out.setBounds(row.removeFromRight(third));
        wet.setBounds(row);

        // gain, then the knee or the time, then the second fold's highpass when it's on, each centred in an equal share
        const auto share = clip.getWidth() / (clipFoldFreq.isVisible() ? 3 : 2);
        clipGain.setBounds(clip.removeFromLeft(share));
        clipFoldFreq.setBounds(clip.removeFromRight(clipFoldFreq.isVisible() ? share : 0));

        for (auto* slider : { &clipKnee, &clipTime })
            slider->setBounds(clip);
    }

private:
    // the limiter keeps the gain and swaps the knee for its release time, the second fold adds its highpass
    void showClipControls()
    {
        const auto index = clipType.getSelectedItemIndex();
        clipKnee.setVisible(index != 1);
        clipTime.setVisible(index == 1);
        clipFoldFreq.setVisible(index == 3);
        resized();
    }

    // flush with the bottom and sides of the window, with the gap over it that the boxes above would otherwise leave
    juce::Rectangle<int> getBar() const { return getLocalBounds().withTrimmedTop(pillInset); }

    static constexpr SlotId clipSlot { ModuleId::postClip, 0 };

    static constexpr int edgePad = 12;
    static constexpr int gap = 6;
    static constexpr int gainsPadding = 14;
    static constexpr int powerSize = 15;
    static constexpr int dotSize = 10;
    static constexpr int clipTypeWidth = 84;
    static constexpr int arrowOnlyWidth = 26; // the arrow HamburgerLAF draws, the name starting past it
    static constexpr int clipReadoutsWidth = 230;
    static constexpr float dividerInset = 8.0f;

    TextSlider in, wet, out, clipGain, clipKnee, clipTime, clipFoldFreq;
    ClipIndicator clipDot;
    juce::ComboBox clipType;
    std::unique_ptr<juce::ParameterAttachment> clipTypeAttachment;

    LightButton power { powerGlyph(), &Theme::powerOn, &Theme::powerOff };
    LightButton clipPower { powerGlyph(), &Theme::powerOn, &Theme::powerOff };
    juce::AudioProcessorValueTreeState::ButtonAttachment powerAttachment, clipPowerAttachment;

    int dividerX = 0;
    bool compact = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginFooter)
};
