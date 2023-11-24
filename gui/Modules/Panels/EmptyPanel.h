#pragma once

 

#include "../Panel.h"
#include "PanelNames.h"

class EmptyPanel : public Panel
{
public:
    EmptyPanel(AudioPluginAudioProcessor &p) : Panel(p, "EMPTY") {
    }
};