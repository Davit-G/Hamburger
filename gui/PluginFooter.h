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

    explicit PluginFooter (AudioPluginAudioProcessor& p)
        : in (p, "IN", ParamIDs::inputGain),
          wet (p, "WET", ParamIDs::mix),
          out (p, "OUT", ParamIDs::outputGain),
          clipGain (p, "GAIN", clipSlot, ParamIDs::postClipGain, ScopeContextType::CLIPPER),
          clipKnee (p, "KNEE", clipSlot, ParamIDs::postClipKnee, ScopeContextType::CLIPPER),
          clipTime (p, "TIME", clipSlot, ParamIDs::postClipTime, ScopeContextType::CLIPPER),
          clipDot (p.getScopeDataCollector(), p),
          powerAttachment (p.treeState, ParamIDs::hamburgerEnabled.getParamID(), power),
          clipPowerAttachment (p.treeState, clipSlot.enabled().getParamID(), clipPower)
    {
        power.setTooltip ("Enable / disable hamburger.");
        clipPower.setTooltip ("Enable / disable the clipper at the end of the chain.");

        auto* link = p.treeState.getParameter (ParamIDs::gainLink.getParamID());
        in.setGainLink (link, false);
        out.setGainLink (link, true);

        in.setColourByGain (true);
        out.setColourByGain (true);
        clipGain.setColourByGain (true);

        for (auto* slider : { &clipGain, &clipKnee, &clipTime })
            slider->showValueOnHover();

        wet.setJustification (juce::Justification::centred);
        out.setJustification (juce::Justification::centredRight);

        for (auto* control : std::initializer_list<GenericKnob*> { &in, &wet, &out, &clipGain, &clipKnee, &clipTime })
        {
            control->setAccent (&Theme::footer);
            addChildComponent (control);
        }

        for (auto* control : std::initializer_list<GenericKnob*> { &in, &wet, &out, &clipGain })
            control->setVisible (true);

        clipType.addItemList (ParamIDs::clipTypes.categories, 1);
        clipType.setTooltip ("How the end of the chain is kept under 0db: a soft clip, or a limiter that sees each peak coming and turns it down smoothly");

        /*  By index, not a ComboBoxAttachment: that spreads the parameter's 0 to 1 over the menu's items, and the menu has
            fewer items than the parameter has choices, so the limiter was landing on the last reserved choice instead */
        clipTypeAttachment = std::make_unique<juce::ParameterAttachment> (*p.treeState.getParameter (clipSlot.type().getParamID()), [this] (float index)
        {
            clipType.setSelectedItemIndex ((int) index, juce::dontSendNotification);
            showClipControls();
        });

        clipType.onChange = [this]
        {
            clipTypeAttachment->setValueAsCompleteGesture ((float) clipType.getSelectedItemIndex());
            showClipControls();
        };

        clipTypeAttachment->sendInitialUpdate();

        for (auto* child : std::initializer_list<juce::Component*> { &power, &clipPower, &clipType, &clipDot })
            addAndMakeVisible (child);
    }

    void lookAndFeelChanged() override
    {
        clipType.setColour (juce::ComboBox::textColourId, theme().footer.text);
        clipType.setColour (juce::ComboBox::arrowColourId, theme().footer.text);
    }

    void paint (juce::Graphics& g) override
    {
        const auto bar = getBar().toFloat();

        g.setColour (theme().box);
        g.fillRect (bar);

        g.setColour (theme().headerDivider);
        g.drawLine ((float) dividerX, bar.getY() + dividerInset, (float) dividerX, bar.getBottom() - dividerInset, 1.0f);
    }

    void resized() override
    {
        auto row = getBar().reduced (pillInset + edgePad, 0);

        auto clip = row.removeFromLeft (row.getWidth() / 2);
        dividerX = clip.getRight();

        clipPower.setBounds (clip.removeFromLeft (powerSize).withSizeKeepingCentre (powerSize, powerSize));
        clip.removeFromLeft (gap);
        // room either side, so the dot doesn't crowd the knee or the divider
        clip.removeFromRight (gap);
        clipDot.setBounds (clip.removeFromRight (dotSize).withSizeKeepingCentre (dotSize, dotSize));
        clip.removeFromRight (gap * 2);

        // the type menu as wide as there's room for once the readouts have theirs
        clipType.setBounds (clip.removeFromLeft (juce::jlimit (clipTypeMinWidth, clipTypeMinWidth + clipTypeWidth, clip.getWidth() - clipReadoutsWidth)));

        row.removeFromLeft (edgePad);

        power.setBounds (row.removeFromLeft (powerSize).withSizeKeepingCentre (powerSize, powerSize));
        row.removeFromLeft (gap);

        const auto third = row.getWidth() / 3;
        in.setBounds (row.removeFromLeft (third));
        out.setBounds (row.removeFromRight (third));
        wet.setBounds (row);

        clipGain.setBounds (clip.removeFromLeft (clip.getWidth() / 2));

        for (auto* slider : { &clipKnee, &clipTime })
            slider->setBounds (clip);
    }

private:
    // the limiter keeps the gain and swaps the knee for its release time
    void showClipControls()
    {
        const auto soft = clipType.getSelectedItemIndex() <= 0;
        clipKnee.setVisible (soft);
        clipTime.setVisible (! soft);
    }

    // flush with the bottom and sides of the window, with the gap over it that the boxes above would otherwise leave
    juce::Rectangle<int> getBar() const { return getLocalBounds().withTrimmedTop (pillInset); }

    static constexpr SlotId clipSlot { ModuleId::postClip, 0 };

    static constexpr int edgePad = 12;
    static constexpr int gap = 6;
    static constexpr int powerSize = 15;
    static constexpr int dotSize = 10;
    static constexpr int clipTypeMinWidth = 60;
    static constexpr int clipTypeWidth = 90;
    static constexpr int clipReadoutsWidth = 170;
    static constexpr float dividerInset = 8.0f;

    TextSlider in, wet, out, clipGain, clipKnee, clipTime;
    ClipIndicator clipDot;
    juce::ComboBox clipType;
    std::unique_ptr<juce::ParameterAttachment> clipTypeAttachment;

    LightButton power { powerGlyph(), &Theme::powerOn, &Theme::powerOff };
    LightButton clipPower { powerGlyph(), &Theme::powerOn, &Theme::powerOff };
    juce::AudioProcessorValueTreeState::ButtonAttachment powerAttachment, clipPowerAttachment;

    int dividerX = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginFooter)
};
