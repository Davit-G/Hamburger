#pragma once

#include "../Panel.h"
#include "PanelNames.h"

class PostClipPanel : public Panel
{
public:
    PostClipPanel(AudioPluginAudioProcessor &p) : Panel(p, "CLIPPER"),
    gain(p, "GAIN", "postClipGain"),
    knee(p, "KNEE", "postClipKnee") {
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