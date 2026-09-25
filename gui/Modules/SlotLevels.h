#pragma once

#include "../TextSlider.h"

class SlotLevels : public juce::Component
{
public:
    SlotLevels (AudioPluginAudioProcessor& p, SlotId slot)
        : in (p, "IN", slot, ParamIDs::slotInGain, ScopeContextType::IN_OUT),
          wet (p, "WET", slot, ParamIDs::slotMix, ScopeContextType::IN_OUT),
          out (p, "OUT", slot, ParamIDs::slotOutGain, ScopeContextType::IN_OUT)
    {
        for (auto* readout : { &in, &wet, &out })
        {
            readout->setFontScale (fontScale);

            // twice JUCE's drag distance over the whole range: the gains span 96 dB, and at the default a quick flick jumped them
            readout->setMouseDragSensitivity (readout->getMouseDragSensitivity() * 2);
            addAndMakeVisible (readout);
        }

        auto* link = p.treeState.getParameter (paramIdFor (slot, ParamIDs::gainLink).getParamID());
        in.setGainLink (link, false);
        out.setGainLink (link, true);

        in.setColourByGain (true);
        out.setColourByGain (true);

        wet.setJustification (juce::Justification::centred);
        out.setJustification (juce::Justification::centredRight);
    }

    void setAccent (AccentColours Theme::* accent)
    {
        for (auto* readout : { &in, &wet, &out })
            readout->setAccent (accent);

        in.refreshText();
        out.refreshText();
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        
        const auto third = bounds.getWidth() / 3;

        in.setBounds (bounds.removeFromLeft (third));
        out.setBounds (bounds.removeFromRight (third));
        wet.setBounds (bounds);
    }

    static constexpr int height = 16;

private:
    static constexpr float fontScale = 0.85f;

    TextSlider in, wet, out;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SlotLevels)
};
