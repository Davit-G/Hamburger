#pragma once

#include "juce_gui_basics/juce_gui_basics.h"
#include "BinaryData.h"

// dont make these static, otherwise due to various lifecycle related issues, will crash on linux
class FontLAF : LookAndFeel_V4
{
public:
    FontLAF() {
        questrialFont12->setHeight(12.0f);
        questrialFont14->setHeight(14.0f);
        questrialFont16->setHeight(16.0f);

        // dont use setsizeandstyle, it will remove the typeface info for windows specifically (weird bug)

        #if JUCE_WINDOWS
            questrialFont12->setHeight(18.0f);
            questrialFont14->setHeight(20.0f);
            questrialFont16->setHeight(22.0f);
        #else
            questrialFont12->setHeight(12.0f);
            questrialFont14->setHeight(14.0f);
            questrialFont16->setHeight(16.0f);
        #endif
    }


    const juce::Typeface::Ptr questrialTypeface = juce::Typeface::createSystemTypefaceFor(BinaryData::QuestrialRegular_ttf, BinaryData::QuestrialRegular_ttfSize);
    const juce::Typeface::Ptr quicksandTypeface = juce::Typeface::createSystemTypefaceFor(BinaryData::QuicksandBold_ttf, BinaryData::QuicksandBold_ttfSize);

    const std::unique_ptr<Font> questrialFont = std::make_unique<juce::Font>(questrialTypeface);
    const std::unique_ptr<Font> questrialFont12 = std::make_unique<juce::Font>(questrialTypeface);
    const std::unique_ptr<Font> questrialFont14 = std::make_unique<juce::Font>(questrialTypeface);
    const std::unique_ptr<Font> questrialFont16 = std::make_unique<juce::Font>(questrialTypeface);


    const std::unique_ptr<Font> quicksandFont = std::make_unique<juce::Font>(quicksandTypeface);
};

class HamburgerFonts {
public:
    static std::shared_ptr<FontLAF> getFontLAF() {
        if (HamburgerFonts::fontLAF == nullptr) {
            HamburgerFonts::fontLAF = std::make_shared<FontLAF>();
        }

        return fontLAF;
    }

private:
    static std::shared_ptr<FontLAF> fontLAF;
};

std::shared_ptr<FontLAF> HamburgerFonts::fontLAF = nullptr;