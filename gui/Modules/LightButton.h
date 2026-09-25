#pragma once

#include "../LookAndFeel/Theme.h"

// stroked in a 24 by 24 box, like the svgs they came from
inline const juce::Path& powerGlyph()
{
    static const auto path = juce::Drawable::parseSVGPath ("M5.636 5.636a9 9 0 1 0 12.728 0M12 3v9");
    return path;
}

inline const juce::Path& lockGlyph()
{
    static const auto path = juce::Drawable::parseSVGPath ("M5 11h14a2 2 0 0 1 2 2v7a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-7a2 2 0 0 1 2-2zM7 11V7a5 5 0 0 1 10 0v4");
    return path;
}

// inset from the area so the glow around it isn't cut off
inline void drawGlowingGlyph (juce::Graphics& g, const juce::Path& glyph, juce::Rectangle<float> area, juce::Colour colour)
{
    area = area.reduced (juce::jmin (area.getWidth(), area.getHeight()) * 0.1f);

    const auto scale = juce::jmin (area.getWidth(), area.getHeight()) / 24.0f;
    const auto toArea = juce::RectanglePlacement (juce::RectanglePlacement::centred).getTransformToFit ({ 0.0f, 0.0f, 24.0f, 24.0f }, area);

    juce::Path stroked;
    juce::PathStrokeType (2.5f * scale, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)
        .createStrokedPath (stroked, glyph, toArea);

    juce::DropShadow (colour.withMultipliedAlpha (0.6f), juce::jmax (1, juce::roundToInt (4.0f * scale)), {}).drawForPath (g, stroked);

    g.setColour (colour);
    g.fillPath (stroked);
}

class LightButton : public juce::Button
{
public:
    LightButton (const juce::Path& glyphToDraw, juce::Colour Theme::* onColour, juce::Colour Theme::* offColour)
        : Button ("light"), glyph (glyphToDraw), on (onColour), off (offColour)
    {
        setClickingTogglesState (true);
    }

    void paintButton (juce::Graphics& g, bool, bool) override
    {
        drawGlowingGlyph (g, glyph, getLocalBounds().toFloat(), theme().*(getToggleState() ? on : off));
    }

private:
    const juce::Path& glyph;
    juce::Colour Theme::* on;
    juce::Colour Theme::* off;
};
