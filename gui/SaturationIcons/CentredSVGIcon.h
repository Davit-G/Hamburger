#pragma once

#include "BinaryData.h"
#include "juce_gui_basics/juce_gui_basics.h"

class CentredSVGIcon : public juce::Component {
public:
    CentredSVGIcon(const char* data, size_t numBytes, int x = 0, int y = 0) {
        drawable = juce::Drawable::createFromImageData(data, numBytes);

        addAndMakeVisible(drawable.get());
        drawable->setInterceptsMouseClicks(false, false);
        setInterceptsMouseClicks(false, false);

        offsetX = x;
        offsetY = y;
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

    std::unique_ptr<juce::Drawable> drawable = nullptr;
};