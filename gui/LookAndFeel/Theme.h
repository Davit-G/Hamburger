#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

#include <optional>

// everything a module type colours its controls with
struct AccentColours
{
    juce::Colour main, mainHeld, icon;
};

// how every module looked before theming, all one colour
inline AccentColours accentFrom(juce::Colour colour)
{
    return { colour, colour, colour };
}

/*  Every colour the plugin draws with, the defaults being the default theme. Components read these straight from
    theme() when they paint, so a new colour is a member here plus a line in the table in Theme.cpp. */
struct Theme
{
    // distortions
    AccentColours grill = accentFrom(juce::Colour(0xffff5977));
    AccentColours tube = accentFrom(juce::Colour(0xffff2e97));
    AccentColours phase = accentFrom(juce::Colour(0xff00e58a));
    AccentColours rubidium = accentFrom(juce::Colour(0xffff924b));
    AccentColours tape = accentFrom(juce::Colour(0xffffac4d));
    AccentColours slew = accentFrom(juce::Colour(0xffffda45));
    AccentColours waveshape = accentFrom(juce::Colour(0xff00e58a));
    AccentColours stack = accentFrom(juce::Colours::white);
    AccentColours start = accentFrom(juce::Colour(0xffff5977));
    juce::Colour startDriveRing = juce::Colours::grey;

    juce::Colour waveshapeAnalog = juce::Colour(0xffff924b);
    juce::Colour waveshapeDigital = juce::Colour(0xff3b78ff);
    juce::Colour waveshapeFolding = juce::Colour(0xff00e58a);
    juce::Colour waveshapeHeavy = juce::Colour(0xffff2e97);
    juce::Colour waveshapeRingFill = juce::Colours::white.withAlpha(0.06f);
    juce::Colour waveshapeRingOutline = juce::Colours::white.withAlpha(0.25f);
    juce::Colour waveshapeHandle = juce::Colours::white;
    juce::Colour waveshapeBlendLine = juce::Colours::white;
    juce::Colour waveshapePillText = juce::Colours::black;

    // other modules
    // one set each for every compressor, pre fx, noise and emphasis type
    AccentColours comp = accentFrom(juce::Colour(0xff00e58a));
    AccentColours preFx = accentFrom(juce::Colour(0xffff924b));
    AccentColours noise = accentFrom(juce::Colour(0xffffda45));
    AccentColours emphasis = accentFrom(juce::Colour(0xffff924b));

    AccentColours footer = accentFrom(juce::Colour(0xffff2e97));

    juce::Colour clipDotIdle = juce::Colours::darkgrey;
    juce::Colour clipKnee = juce::Colours::orange;
    juce::Colour clipHard = juce::Colours::red;

    // scope
    juce::Colour scopeBackground = juce::Colours::black;
    juce::Colour scopeGrid = juce::Colours::grey.withAlpha(0.6f);
    juce::Colour scopeLeft = juce::Colours::yellow;
    juce::Colour scopeRight = juce::Colours::lime;
    juce::Colour spectrumLine = juce::Colour(0xff999900);
    juce::Colour spectrumFill = juce::Colour(0xff999900).withAlpha(0.12f);
    juce::Colour scopeCurve = juce::Colours::grey;
    juce::Colour scopeCurvePost = juce::Colours::white;
    juce::Colour scopeWatermark = juce::Colour::fromRGBA(255, 255, 255, 20);
    juce::Colour scopeHeader = juce::Colours::black;

    juce::Colour scopeClipKneeRegion = juce::Colour::fromRGBA(255, 255, 255, 22);
    juce::Colour scopeClipFill = juce::Colours::white.withAlpha(0.05f);
    juce::Colour scopeClipBelow = juce::Colours::yellow;

    juce::Colour compCell = juce::Colours::black;
    juce::Colour compOverThreshold = juce::Colour(0xff161616);
    juce::Colour compStackedOverThreshold = juce::Colour(0xff1e1e1e);
    juce::Colour compKnee = juce::Colour(0xff262626);
    juce::Colour compLevel = juce::Colours::yellow;
    juce::Colour compLevelOver = juce::Colours::orange;
    juce::Colour compThreshold = juce::Colours::white.withAlpha(0.9f);

    // multiband
    juce::Colour multibandTopEdge = juce::Colour(0xff303030);
    juce::Colour multibandInputSpectrum = juce::Colours::white.withAlpha(0.6f);
    juce::Colour multibandInGain = juce::Colours::grey.withAlpha(0.4f);
    juce::Colour multibandLevel = juce::Colours::white.withAlpha(0.4f);
    juce::Colour multibandLevelHighlight = juce::Colours::white.withAlpha(0.9f);
    juce::Colour multibandInactiveShade = juce::Colours::black.withAlpha(0.45f);
    juce::Colour multibandSilencedShade = juce::Colours::black.withAlpha(0.55f);
    juce::Colour multibandBandHighlight = juce::Colours::white.withAlpha(0.05f);
    juce::Colour multibandDrive = juce::Colours::white.withAlpha(0.45f);
    juce::Colour multibandDriveActive = juce::Colours::white.withAlpha(0.7f);
    juce::Colour multibandDriveHot = juce::Colours::white;
    juce::Colour multibandButton = juce::Colour(0xff1e1e1e);
    juce::Colour multibandButtonTextOn = juce::Colours::black;
    juce::Colour multibandMute = juce::Colour(0xffdc4646);
    juce::Colour multibandSolo = juce::Colours::yellow;
    juce::Colour multibandRemove = juce::Colours::grey;
    juce::Colour multibandRemoveHot = juce::Colour(0xffdc4646);
    juce::Colour multibandAddLine = juce::Colours::grey.withAlpha(0.6f);
    juce::Colour multibandAddBadge = juce::Colour(0xff464646);
    juce::Colour multibandAddCross = juce::Colours::white;
    juce::Colour multibandCrossover = juce::Colours::white.withAlpha(0.45f);
    juce::Colour multibandCrossoverHot = juce::Colours::white.withAlpha(0.9f);
    juce::Colour multibandCrossoverHighlight = juce::Colours::white.withAlpha(0.1f);
    juce::Colour multibandCrossoverTextHot = juce::Colours::white;

    // other
    juce::Colour backgroundBase = juce::Colours::white;
    std::array<juce::Colour, 6> backgroundRegions { juce::Colour(0xffffa746), juce::Colour(0xffff5977), juce::Colour(0xffffce00),
                                                    juce::Colour(0xffa9412a), juce::Colour(0xff99ff80), juce::Colour(0xffffa746) };
    std::array<juce::Colour, 6> backgroundLines { juce::Colour(0xff2e2e2e), juce::Colour(0xff2e2e2e), juce::Colour(0xff2e2e2e),
                                                  juce::Colour(0xff2e2e2e), juce::Colour(0xff2e2e2e), juce::Colour(0xff2e2e2e) };

    juce::Colour logoTop = juce::Colour(0xffffb55f);
    juce::Colour logoBottom = juce::Colour(0xffffd056);

    juce::Colour box = juce::Colours::black;
    juce::Colour boxDisabled = juce::Colour::fromRGBA(0, 0, 0, 150);
    bool boxBorders = false;
    juce::Colour boxBorder = juce::Colours::white.withAlpha(0.3f);
    juce::Colour boxBorderDisabled = juce::Colours::white.withAlpha(0.15f);

    juce::Colour knobThumb = juce::Colours::whitesmoke;
    juce::Colour scaledMarker = juce::Colours::whitesmoke.withAlpha(0.7f);
    juce::Colour sliderTrack = juce::Colours::darkgrey;
    juce::Colour powerOn = juce::Colour(0xff98fd9e);
    juce::Colour powerOff = juce::Colour(0xffe65866);
    juce::Colour gainTextLow = juce::Colour(0xff8c8c8c);
    juce::Colour modulationHighlight = juce::Colour(0xff4ef7ff);

    // one per mod source, in the order of ModSources: drive, the macros, the modulators, then keytrack and velocity
    // the drive in the header's orange, the macros the logo burger's layers (tomato, bun), the modulators green, sky and
    // digital blue, and the global sources pink and violet
    std::array<juce::Colour, 8> modSources { juce::Colour(0xffffa746), juce::Colour(0xffff7b7b), juce::Colour(0xffffd056),
                                             juce::Colour(0xff00e58a), juce::Colour(0xff4dc3ff), juce::Colour(0xff3b78ff),
                                             juce::Colour(0xffff2e97), juce::Colour(0xffc58aff) };

    juce::Colour headerSelected = juce::Colour(0xffffa746);
    juce::Colour textHeader = juce::Colours::white;
    juce::Colour textHeaderHover = juce::Colours::white.withAlpha(0.67f);
    juce::Colour textHeaderIdle = juce::Colours::white.withAlpha(0.43f);
    juce::Colour headerDivider = juce::Colour(0xff4a4a4a);
    juce::Colour textDim = juce::Colours::grey;
    juce::Colour textPresets = juce::Colours::white;
    juce::Colour textPresetsAuthor = juce::Colour(0xff646464);

    // every module type shares these, whatever its accent
    juce::Colour categoryText = juce::Colours::white;
    juce::Colour knobText = juce::Colours::white;
    juce::Colour labelText = juce::Colours::white;

    juce::Colour settingsButton = juce::Colour(0xff161616);
    juce::Colour rowDragged = juce::Colour(0xff464646);
    juce::Colour rowSelected = juce::Colour(0xff212121);
    juce::Colour divider = juce::Colour(0xff2c2c2c);
    juce::Colour scrollbar = juce::Colour(0xff00e58a);

    // a shade lighter than the boxes, so they read as sitting on top of them
    juce::Colour popupBackground = juce::Colour(0xff1e1e1e);
    juce::Colour popupText = juce::Colours::white;
    juce::Colour popupHighlight = juce::Colour(0xff383838);
    juce::Colour alertBackground = juce::Colour(0xff1a1a1a);
    juce::Colour alertOutline = juce::Colour(0xff161616);
    juce::Colour alertText = juce::Colours::white;
    juce::Colour alertAccent = juce::Colour(0xffff5977);
    juce::Colour alertAccentText = juce::Colours::black;

    // not themeable, for the panels without controls of their own like the logo and the scope
    AccentColours plain = accentFrom(juce::Colours::whitesmoke);
};

// the theme on screen, shared by every instance of the plugin
inline Theme& theme()
{
    static Theme current;
    return current;
}

struct ThemeColour
{
    juce::String id, name, description;
    std::function<juce::Colour& (Theme&)> in;

    // which module type it belongs to, and the wider group it usually matches, like every compressor
    juce::String type, family;
};

// a heading in the customiser and a block of the ini file
struct ThemeSection
{
    juce::String id, title, description;
    std::vector<ThemeColour> colours;
};

const std::vector<ThemeSection>& themeSections();

struct ThemeInfo
{
    // background is an svg file to draw instead of the built in one
    juce::String name, author, description, background;
};

// hex with or without the #, 3 4 6 or 8 digits with the alpha last, or "r, g, b" and "r, g, b, a" with or without rgb( )
std::optional<juce::Colour> parseColour(juce::String text);

// #RRGGBB, with AA on the end when it isn't opaque
juce::String colourToHex(juce::Colour colour);

juce::String themeToIni(Theme theme, const ThemeInfo& info);

// an svg can name any theme colour by its id where a colour goes, like fill="background_region_1", and gets that colour
void applyThemeToSvg(juce::XmlElement& svg);

// anything the text leaves out stays at the default theme's colour
Theme themeFromIni(const juce::String& text, ThemeInfo& info);
