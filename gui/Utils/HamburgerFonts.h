#pragma once

class HamburgerFonts
{
public:
    static const juce::Typeface::Ptr questrialTypeface;
    static const juce::Typeface::Ptr quicksandTypeface;

    static const std::unique_ptr<Font> questrialFont;
    static const std::unique_ptr<Font> quicksandFont;
};

const juce::Typeface::Ptr HamburgerFonts::questrialTypeface = juce::Typeface::createSystemTypefaceFor(BinaryData::QuestrialRegular_ttf, BinaryData::QuestrialRegular_ttfSize);
const juce::Typeface::Ptr HamburgerFonts::quicksandTypeface = juce::Typeface::createSystemTypefaceFor(BinaryData::QuicksandBold_ttf, BinaryData::QuicksandBold_ttfSize);

const std::unique_ptr<juce::Font, std::default_delete<juce::Font>> HamburgerFonts::questrialFont = std::make_unique<juce::Font>(HamburgerFonts::questrialTypeface);
const std::unique_ptr<juce::Font, std::default_delete<juce::Font>> HamburgerFonts::quicksandFont = std::make_unique<juce::Font>(HamburgerFonts::quicksandTypeface);