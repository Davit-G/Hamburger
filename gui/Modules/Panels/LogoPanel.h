#pragma once

#include "../Panel.h"
#include "PanelNames.h"

#include "../Scope.h"

class LogoPanel : public Panel
{
public:
    LogoPanel(AudioPluginAudioProcessor &p) : Panel(p, "SETTINGS")
    {
    }

    void resized() override
    {
    }

    void paint(Graphics &g) override {
        g.drawImage(image, getLocalBounds().reduced(20).toFloat(), RectanglePlacement::centred);
    }

private:
    juce::Image image = ImageCache::getFromMemory(BinaryData::HamburgerLogoThick_png, BinaryData::HamburgerLogoThick_pngSize);
};