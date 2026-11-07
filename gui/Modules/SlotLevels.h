#pragma once

#include "../TextSlider.h"

class SlotLevels : public juce::Component
{
public:
    // the scope shows the module's own view while these are dragged
    SlotLevels(AudioPluginAudioProcessor& p, SlotId slot, ScopeContextType scopeType = ScopeContextType::IN_OUT)
        : in(p, "IN", slot, ParamIDs::slotInGain, scopeType),
          wet(p, "WET", slot, ParamIDs::slotMix, scopeType),
          out(p, "OUT", slot, ParamIDs::slotOutGain, scopeType)
    {
        for (auto* readout : { &in, &wet, &out })
        {
            readout->setFontScale(fontScale);

            // twice JUCE's drag distance over the whole range: the gains span 96 dB, and at the default a quick flick jumped them
            readout->setMouseDragSensitivity(readout->getMouseDragSensitivity() * 2);
            addAndMakeVisible(readout);
        }

        auto* link = p.treeState.getParameter(paramIdFor(slot, ParamIDs::gainLink).getParamID());
        auto level = [&](const ParamIDs::ParameterInfo& info) { return p.treeState.getParameter(paramIdFor(slot, info).getParamID()); };
        in.setGainLink(link, false, level(ParamIDs::slotOutGain));
        out.setGainLink(link, true, level(ParamIDs::slotInGain));

        in.setColourByGain(true);
        wet.setColourByGain(true);
        out.setColourByGain(true);

        wet.setJustification(juce::Justification::centred);
        out.setJustification(juce::Justification::centredRight);
    }

    void setAccent(AccentColours Theme::* accent)
    {
        for (auto* readout : { &in, &wet, &out })
            readout->setAccent(accent);

        in.refreshText();
        out.refreshText();
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        
        const auto third = bounds.getWidth() / 3;

        // the name and the value side by side where there's room, like "OUT: -12.0 dB", otherwise the name with the
        // value swapped in while it's hovered or dragged, like the footer's
        for (auto* readout : { &in, &wet, &out })
            readout->showValueOnHover(third < fullTextWidth);

        in.setBounds(bounds.removeFromLeft(third));
        out.setBounds(bounds.removeFromRight(third));
        wet.setBounds(bounds);
    }

    static constexpr int height = 16;

private:
    static constexpr float fontScale = 0.85f;
    static constexpr int fullTextWidth = 90; // about what the longest, like "OUT: -24.0 dB", takes

    TextSlider in, wet, out;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SlotLevels)
};
