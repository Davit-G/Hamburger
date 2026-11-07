#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"
#include "../../../dsp/Distortions/DllDistortion.h"
#include "../../Knob.h"
#include "../../RectSlider.h"

// debug builds only: the drive, six knobs for whatever the dylib makes of them, and the dylib to run in the dial's middle
class DllPanel : public Panel, private juce::Timer
{
public:
    DllPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::main, 0}) : Panel(p, "DLL", &Theme::stack, slot),
        treeState(p.treeState),
        drive(p, "DRIVE", slot, ParamIDs::dllDrive, ScopeContextType::IN_OUT),
        a(p, "A", slot, ParamIDs::dllA, ScopeContextType::IN_OUT),
        b(p, "B", slot, ParamIDs::dllB, ScopeContextType::IN_OUT),
        c(p, "C", slot, ParamIDs::dllC, ScopeContextType::IN_OUT),
        d(p, "D", slot, ParamIDs::dllD, ScopeContextType::IN_OUT),
        e(p, "E", slot, ParamIDs::dllE, ScopeContextType::IN_OUT),
        f(p, "F", slot, ParamIDs::dllF, ScopeContextType::IN_OUT)
    {
        addKnobs({&drive, &a, &b, &c, &d, &e, &f});
        addAndMakeVisible(load);

        load.onClick = [this]
        {
            const auto pathKey = DllDistortion::pathKeyFor(this->slot);
            chooser = std::make_unique<juce::FileChooser>("Pick a dylib to hot reload", juce::File(treeState.state[pathKey].toString()), "*.dylib");
            chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this, pathKey](const juce::FileChooser& picked)
            {
                if (picked.getResult() != juce::File())
                    treeState.state.setProperty(pathKey, picked.getResult().getFullPathName(), nullptr);
            });
        };

        timerCallback();
        startTimerHz(4);
    }

    void resized() override
    {
        layoutSaturation(drive, load, {&a, &b, &c, &d, &e, &f});
        load.setBounds(load.getBounds().expanded(load.getWidth() / 2, 0));
    }

private:
    // the dylib's name on the button, and how loading it went on hovering it
    void timerCallback() override
    {
        const juce::File file(treeState.state[DllDistortion::pathKeyFor(slot)].toString());
        load.setButtonText(file == juce::File() ? "LOAD" : file.getFileNameWithoutExtension());
        load.setTooltip(file.getFullPathName() + "\n" + treeState.state[DllDistortion::statusKeyFor(slot)].toString());
    }

    juce::AudioProcessorValueTreeState& treeState;
    ParamKnob drive;
    RectSlider a, b, c, d, e, f;
    juce::TextButton load;
    std::unique_ptr<juce::FileChooser> chooser;
};
