#pragma once

#include "../Panel.h"

// the emphasis box's tilt mode: one knob, tilting the whole spectrum one way before the distortion and back after it
class TiltPanel : public Panel
{
public:
    explicit TiltPanel (AudioPluginAudioProcessor& p)
        : Panel (p, "TILT", &Theme::tilt),
          tilt (p, "TILT", ParamIDs::emphasisTilt, ScopeContextType::SPECTRUM_TILT)
    {
        addAndMakeVisible (tilt);
    }

    void resized() override
    {
        const auto size = juce::jmin (getWidth(), getHeight()) * 3 / 4;
        tilt.setBounds (getLocalBounds().withSizeKeepingCentre (size, size));
    }

private:
    ParamKnob tilt;
};
