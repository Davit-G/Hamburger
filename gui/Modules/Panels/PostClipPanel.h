#pragma once

#include "../Panel.h"

class PostClipPanel : public Panel
{
public:
    PostClipPanel(AudioPluginAudioProcessor &p, SlotId slot = SlotId{ModuleId::postClip, 0}) : Panel(p, "CLIPPER", Palette::colours[4]),
        gain(p, "GAIN", slot, ParamIDs::postClipGain, ValueType::db, ScopeContextType::CLIPPER),
        knee(p, "KNEE", slot, ParamIDs::postClipKnee, ValueType::db, ScopeContextType::CLIPPER)
    {
        addKnobs({&gain, &knee});
    }

    void resized() override { layoutGrid({&gain, &knee}); }

private:
    ParamKnob gain, knee;
};
