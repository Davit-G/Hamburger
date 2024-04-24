#pragma once

#include "juce_gui_basics/juce_gui_basics.h"
#include "BinaryData.h"

// dont make these static, otherwise due to various lifecycle related issues, will crash on linux
class FontLAF : LookAndFeel_V4
{
public:
    const juce::Typeface::Ptr questrialTypeface = juce::Typeface::createSystemTypefaceFor(BinaryData::QuestrialRegular_ttf, BinaryData::QuestrialRegular_ttfSize);
    const juce::Typeface::Ptr quicksandTypeface = juce::Typeface::createSystemTypefaceFor(BinaryData::QuicksandBold_ttf, BinaryData::QuicksandBold_ttfSize);

    const std::unique_ptr<Font> questrialFont = std::make_unique<juce::Font>(questrialTypeface);
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