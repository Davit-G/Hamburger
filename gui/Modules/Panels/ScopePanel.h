#pragma once

#include "../Panel.h"


#include "../Scope.h"

class ScopePanel : public Panel
{
public:
    ScopePanel(AudioPluginAudioProcessor &p) : Panel(p, "SETTINGS"),
                                                  scope(p.getAudioBufferQueueL(), p.getAudioBufferQueueR())
    {
        addAndMakeVisible (scope);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        scope.setBounds(bounds);
    }

private:

    Scope<float> scope;
};