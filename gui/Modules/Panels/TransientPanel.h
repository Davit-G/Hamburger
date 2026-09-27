#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"

// both transient shapers, the multiband one with a tilt where the other has its stereo link
class TransientPanel : public Panel
{
public:
    TransientPanel(AudioPluginAudioProcessor &p, bool multiband)
        : Panel(p, multiband ? "MB TRANSIENT" : "TRANSIENT", multiband ? &Theme::multibandTransient : &Theme::transient),
          attack(p, "ATTACK", compSlot, ParamIDs::transientAttack, ScopeContextType::COMPRESSION),
          sustain(p, "SUSTAIN", compSlot, ParamIDs::transientSustain, ScopeContextType::COMPRESSION),
          speed(p, "SPEED", compSlot, ParamIDs::transientSpeed, ScopeContextType::COMPRESSION),
          linkOrTilt(p, multiband ? "TILT" : "LINK", compSlot, multiband ? ParamIDs::transientTilt : ParamIDs::transientLink, ScopeContextType::COMPRESSION)
    {
        addAndMakeVisible(attack);
        addAndMakeVisible(sustain);
        addAndMakeVisible(speed);
        addAndMakeVisible(linkOrTilt);
    }

    void resized() override
    {
        using fr = juce::Grid::Fr;
        using Track = juce::Grid::TrackInfo;

        grid.templateRows = {Track(fr(1)), Track(fr(1))};
        grid.templateColumns = {Track(fr(1)), Track(fr(1))};

        grid.items = {
            juce::GridItem(attack).withArea(1, 1),
            juce::GridItem(sustain).withArea(1, 2),
            juce::GridItem(speed).withArea(2, 1),
            juce::GridItem(linkOrTilt).withArea(2, 2)};

        grid.performLayout(getLocalBounds());
    }

private:
    static constexpr SlotId compSlot { ModuleId::dynamics, 0 };

    juce::Grid grid;

    ParamKnob attack;
    ParamKnob sustain;
    ParamKnob speed;
    ParamKnob linkOrTilt;
};
