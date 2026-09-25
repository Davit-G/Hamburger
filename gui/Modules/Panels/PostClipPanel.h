#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"

class PostClipPanel : public Panel
{
public:
    PostClipPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::postClip, 0}) : Panel(p, "CLIPPER", &Theme::tube),
    gain(p, "GAIN", slot, ParamIDs::postClipGain, ScopeContextType::CLIPPER),
    knee(p, "KNEE", slot, ParamIDs::postClipKnee, ScopeContextType::CLIPPER) {
        addAndMakeVisible(gain);
        addAndMakeVisible(knee);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        auto width = bounds.getWidth() / 2;
        gain.setBounds(bounds.removeFromLeft(width));
        knee.setBounds(bounds.removeFromLeft(width));
    }

private:
    ParamKnob gain;
    ParamKnob knee;
};