#pragma once

#include "LookAndFeel/ThemeManager.h"
#include "LookAndFeel/HamburgerLAF.h"

// hamburger's fonts on black, with fixed greys rather than the theme's so the window stays readable whatever the theme is
class CustomiserLookAndFeel : public HamburgerLAF
{
public:
    CustomiserLookAndFeel()
    {
        const auto black = juce::Colours::black, white = juce::Colours::white;
        const auto dark = juce::Colour (0xff161616), grey = juce::Colour (0xff464646);

        setColourScheme ({ black, dark, black, juce::Colour (0xff2c2c2c), white, grey, white, grey, white });

        setColour (juce::ColourSelector::backgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::ColourSelector::labelTextColourId, white);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int) override { return getQuicksandFont(); }
};

// the hex written in a box of its own colour, click it to type or paste a new one
class ColourSwatch : public juce::Label
{
public:
    ColourSwatch()
    {
        setEditable (true, false, false);
        setJustificationType (juce::Justification::centred);
    }

    std::function<void (juce::Colour)> onTyped;

    void show (juce::Colour newColour)
    {
        colour = newColour;

        // judged over the checkerboard, so a see through colour still gets readable text
        const auto seen = juce::Colour (0xff808080).overlaidWith (colour);

        setColour (textColourId, seen.getPerceivedBrightness() > 0.5f ? juce::Colours::black : juce::Colours::white);
        setText (colourToHex (colour), juce::dontSendNotification);
        repaint();
    }

    // drawn here rather than by the label, which would write it in the regular font
    void paint (juce::Graphics& g) override
    {
        const auto area = getLocalBounds().toFloat();

        g.fillCheckerBoard (area, 6.0f, 6.0f, juce::Colour (0xff999999), juce::Colour (0xff666666));
        g.setColour (colour);
        g.fillRect (area);

        if (isBeingEdited())
            return;

        if (auto* laf = dynamic_cast<HamburgerLAF*> (&getLookAndFeel()))
            g.setFont (laf->getQuicksandFont());

        g.setColour (findColour (textColourId));
        g.drawText (getText(), area, juce::Justification::centred, false);
    }

private:
    void editorShown (juce::TextEditor* editor) override { editor->selectAll(); }

    void textWasEdited() override
    {
        if (auto parsed = parseColour (getText()))
            onTyped (*parsed);
        else
            show (colour);
    }

    juce::Colour colour;
};

class ColourPicker : public juce::ColourSelector,
                     private juce::ChangeListener
{
public:
    ColourPicker (juce::Colour start, std::function<void (juce::Colour)> onPicked)
        : juce::ColourSelector (showAlphaChannel | showColourAtTop | editableColour | showSliders | showColourspace),
          picked (std::move (onPicked))
    {
        setCurrentColour (start, juce::dontSendNotification);
        addChangeListener (this);
        setSize (300, 380);
    }

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override { picked (getCurrentColour()); }

    std::function<void (juce::Colour)> picked;
};

// an icon stroked in a 24 by 24 box, like the svgs it comes from
class GlyphButton : public juce::Button
{
public:
    GlyphButton (const juce::String& svgPath, const juce::String& tooltip)
        : juce::Button (tooltip), glyph (juce::Drawable::parseSVGPath (svgPath))
    {
        setTooltip (tooltip);
    }

    void paintButton (juce::Graphics& g, bool highlighted, bool) override
    {
        g.setColour (highlighted ? juce::Colours::white : juce::Colours::grey);
        g.strokePath (glyph, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                      juce::RectanglePlacement (juce::RectanglePlacement::centred)
                          .getTransformToFit ({ 0.0f, 0.0f, 24.0f, 24.0f }, getLocalBounds().toFloat().reduced (4.0f)));
    }

private:
    juce::Path glyph;
};

// every edit made in the window as the whole theme before and after it, so undoing puts back exactly what was there
class ThemeHistory
{
public:
    explicit ThemeHistory (ThemeManager& manager) : themes (manager) {}

    // a new undo step, unless it carries on the last one like every move of one picker drag does
    void change (const std::function<void (Theme&)>& edit, bool continuesLastStep = false)
    {
        if (! continuesLastStep)
            undoManager.beginNewTransaction();

        auto after = theme();
        edit (after);
        undoManager.perform (new Edit (theme(), after, themes));
    }

    void undo() { undoManager.undo(); }
    void redo() { undoManager.redo(); }
    bool canUndo() const { return undoManager.canUndo(); }
    bool canRedo() const { return undoManager.canRedo(); }
    void clear() { undoManager.clearUndoHistory(); }

private:
    struct Edit : public juce::UndoableAction
    {
        Edit (const Theme& themeBefore, const Theme& themeAfter, ThemeManager& manager)
            : before (themeBefore), after (themeAfter), themes (manager) {}

        bool perform() override { return show (after); }
        bool undo() override { return show (before); }

        bool show (const Theme& shown)
        {
            theme() = shown;
            themes.edited();
            return true;
        }

        Theme before, after;
        ThemeManager& themes;
    };

    ThemeManager& themes;
    juce::UndoManager undoManager { 10000, 100 };
};

class WheelButton : public juce::Button
{
public:
    WheelButton() : juce::Button ("pick") { setTooltip ("Pick with a colour wheel"); }

    void paintButton (juce::Graphics& g, bool highlighted, bool) override
    {
        const auto wheel = getLocalBounds().toFloat().reduced (highlighted ? 2.0f : 3.0f);
        constexpr int slices = 24;

        for (int i = 0; i < slices; ++i)
        {
            juce::Path slice;
            slice.addPieSegment (wheel, juce::MathConstants<float>::twoPi * (float) i / slices,
                                 juce::MathConstants<float>::twoPi * (float) (i + 1) / slices, 0.0f);

            g.setColour (juce::Colour::fromHSV ((float) i / slices, 0.8f, 1.0f, 1.0f));
            g.fillPath (slice);
        }
    }
};
