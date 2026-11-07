#pragma once

#include "BinaryData.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "../LookAndFeel/Theme.h"

class CentredSVGIcon : public juce::Component {
public:
    CentredSVGIcon(const char* data, size_t numBytes, AccentColours Theme::* accentColours, int x = 0, int y = 0)
        : svgData(data), svgSize(numBytes), accent(accentColours) {
        setInterceptsMouseClicks(false, false);

        offsetX = x;
        offsetY = y;
    }

    // the svgs are drawn in the default theme's icon colour, so that's the colour swapped out for the current one
    void lookAndFeelChanged() override {
        if (drawable != nullptr && drawnColour == (theme().*accent).icon)
            return;

        drawnColour = (theme().*accent).icon;
        drawable = juce::Drawable::createFromImageData(svgData, svgSize);
        drawable->replaceColour((Theme().*accent).icon, (theme().*accent).icon);
        drawable->setInterceptsMouseClicks(false, false);
        addAndMakeVisible(drawable.get());
        resized();
    }

    void resized() override {
        if (drawable != nullptr)
        {   
            juce::Rectangle<float> areaInParent = getLocalBounds().translated(offsetX, offsetY).toFloat();
            drawable->setTransformToFit (areaInParent, juce::RectanglePlacement::centred);
        }
    }

private:
    int offsetX, offsetY;

    const char* svgData;
    size_t svgSize;
    AccentColours Theme::* accent;
    juce::Colour drawnColour;

    std::unique_ptr<juce::Drawable> drawable = nullptr;
};
