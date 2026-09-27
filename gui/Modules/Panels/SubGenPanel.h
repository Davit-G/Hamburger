#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"

class SubGenPanel : public Panel
{
public:
    explicit SubGenPanel(AudioPluginAudioProcessor &p) : Panel(p, "SUB GEN", &Theme::subGen),
        amount(p, "SUB", SlotId{ModuleId::module2, 0}, ParamIDs::subGenAmount)
    {
        addAndMakeVisible(amount);
    }

    void resized() override { amount.setBounds(getLocalBounds()); }

private:
    ParamKnob amount;
};

class HilbertStackPanel : public Panel
{
public:
    explicit HilbertStackPanel(AudioPluginAudioProcessor &p) : Panel(p, "HILBERT", &Theme::hilbertStack),
        stacks(p, "STACKS", SlotId{ModuleId::module2, 0}, ParamIDs::hilbertStacks)
    {
        addAndMakeVisible(stacks);
    }

    void resized() override { stacks.setBounds(getLocalBounds()); }

private:
    ParamKnob stacks;
};
