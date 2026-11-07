#pragma once

#include "../Panel.h"
#include "../../../dsp/EffectInfos.h"

// both transient shapers, the multiband one with a tilt where the other has its stereo link
class TransientPanel : public Panel
{
public:
    TransientPanel(AudioPluginAudioProcessor &p, bool multiband)
        : Panel(p, multiband ? "MB TRANSIENT" : "TRANSIENT", multiband ? &Theme::comp : &Theme::comp),
          attack(p, "ATTACK", compSlot, ParamIDs::transientAttack, ScopeContextType::COMPRESSION),
          sustain(p, "SUSTAIN", compSlot, ParamIDs::transientSustain, ScopeContextType::COMPRESSION),
          speed(p, "SPEED", compSlot, ParamIDs::transientSpeed, ScopeContextType::COMPRESSION),
          linkOrTilt(p, multiband ? "TILT" : "LINK", compSlot, multiband ? ParamIDs::transientTilt : ParamIDs::transientLink, ScopeContextType::COMPRESSION)
    {
        addKnobs({&attack, &sustain, &speed, &linkOrTilt});
    }

    void resized() override { layoutGrid({&attack, &sustain, &speed, &linkOrTilt}, 2); }

private:
    static constexpr SlotId compSlot { ModuleId::dynamics, 0 };

    ParamKnob attack, sustain, speed, linkOrTilt;
};
