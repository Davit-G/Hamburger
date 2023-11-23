#pragma once

#include <JuceHeader.h>

#include "../Panel.h"
#include "PanelNames.h"

class EmptyPanel : public Panel
{
public:
    EmptyPanel(AudioPluginAudioProcessor &p) : Panel(p, "EMPTY") {
    }
};