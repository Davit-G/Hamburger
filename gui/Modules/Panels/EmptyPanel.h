#pragma once

 

#include "../Panel.h"


class EmptyPanel : public Panel
{
public:
    EmptyPanel(AudioPluginAudioProcessor &p) : Panel(p, "EMPTY") {
    }
};