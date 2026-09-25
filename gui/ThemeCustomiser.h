#pragma once

#include "ThemeCustomiserParts.h"
#include "BoxToggle.h"

class ColourRow : public juce::Component
{
public:
    ColourRow (const ThemeColour& themeColour, ThemeHistory& themeHistory) : entry (themeColour), history (themeHistory)
    {
        name.setText (entry.name, juce::dontSendNotification);
        name.setTooltip (entry.description);
        addAndMakeVisible (name);

        swatch.setTooltip (entry.description + ". Right click for more");
        swatch.onTyped = [this] (juce::Colour colour) { set (colour); };
        addAndMakeVisible (swatch);

        // right clicks on the name or the swatch open the row's menu
        name.addMouseListener (this, false);
        swatch.addMouseListener (this, false);

        copy.onClick = [this] { juce::SystemClipboard::copyTextToClipboard (colourToHex (entry.in (theme()))); };
        addAndMakeVisible (copy);

        paste.onClick = [this]
        {
            if (auto pasted = parseColour (juce::SystemClipboard::getTextFromClipboard()))
                set (*pasted);
        };
        addAndMakeVisible (paste);

        wheel.onClick = [this]
        {
            // one undo step for the whole time the picker is open
            auto picker = std::make_unique<ColourPicker> (entry.in (theme()), [row = SafePointer<ColourRow> (this), firstMove = true] (juce::Colour colour) mutable
            {
                if (row != nullptr)
                    row->set (colour, ! std::exchange (firstMove, false));
            });

            // inside the window, so it's drawn with the window's look and feel
            auto* window = getTopLevelComponent();
            juce::CallOutBox::launchAsynchronously (std::move (picker), window->getLocalArea (&wheel, wheel.getLocalBounds()), window);
        };
        addAndMakeVisible (wheel);

        refresh();
    }

    void refresh() { swatch.show (entry.in (theme())); }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (0, 2);

        wheel.setBounds (bounds.removeFromRight (bounds.getHeight()));
        paste.setBounds (bounds.removeFromRight (bounds.getHeight()));
        copy.setBounds (bounds.removeFromRight (bounds.getHeight()));
        bounds.removeFromRight (4);
        swatch.setBounds (bounds.removeFromRight (110));
        name.setBounds (bounds);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! e.mods.isPopupMenu())
            return;

        juce::PopupMenu menu;
        menu.setLookAndFeel (&getLookAndFeel());

        menu.addItem ("Copy", [this] { copy.onClick(); });
        menu.addItem ("Paste", parseColour (juce::SystemClipboard::getTextFromClipboard()).has_value(), false, [this] { paste.onClick(); });

        for (const auto& group : juce::StringArray { entry.type, entry.family })
        {
            const auto matches = matching (group);

            if (group.isNotEmpty() && ! matches.empty())
                menu.addItem ("Copy to matching " + group + " colours", [this, matches]
                {
                    const auto colour = entry.in (theme());
                    history.change ([&] (Theme& t) { for (auto* match : matches) match->in (t) = colour; });
                });
        }

        menu.showMenuAsync (juce::PopupMenu::Options().withMousePosition());
    }

private:
    void set (juce::Colour colour, bool continuesLastStep = false)
    {
        history.change ([&] (Theme& t) { entry.in (t) = colour; }, continuesLastStep);
        refresh();
    }

    // the rest of the type or family that shares this colour in the default theme, like every compressor's knob
    std::vector<const ThemeColour*> matching (const juce::String& group) const
    {
        Theme defaults;
        std::vector<const ThemeColour*> matches;

        for (const auto& section : themeSections())
            for (const auto& colour : section.colours)
                if (&colour != &entry && (colour.type == group || colour.family == group) && colour.in (defaults) == entry.in (defaults))
                    matches.push_back (&colour);

        return matches;
    }

    const ThemeColour& entry;
    ThemeHistory& history;

    juce::Label name;
    ColourSwatch swatch;
    GlyphButton copy { "M10 8h10a2 2 0 0 1 2 2v10a2 2 0 0 1-2 2H10a2 2 0 0 1-2-2V10a2 2 0 0 1 2-2zM4 16a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h10a2 2 0 0 1 2 2", "Copy" };
    GlyphButton paste { "M16 4h2a2 2 0 0 1 2 2v14a2 2 0 0 1-2 2H6a2 2 0 0 1-2-2V6a2 2 0 0 1 2-2h2M9 2h6a1 1 0 0 1 1 1v2a1 1 0 0 1-1 1H9a1 1 0 0 1-1-1V3a1 1 0 0 1 1-1z", "Paste" };
    WheelButton wheel;
};

// every colour in one long column, under a heading per section
class ColourList : public juce::Component
{
public:
    explicit ColourList (ThemeHistory& themeHistory) : history (themeHistory)
    {
        borders.setTooltip ("Draws an outline around every box, in the box border colours");
        borders.onClick = [this] { history.change ([this] (Theme& t) { t.boxBorders = borders.getToggleState(); }); };

        for (const auto& section : themeSections())
        {
            auto* heading = headings.add (new juce::Label ({}, section.title));
            HamburgerLAF::setLabelFontScale (*heading, 1.4f);
            heading->setTooltip (section.description);
            addAndMakeVisible (heading);

            if (section.id == "other")
                addAndMakeVisible (borders);

            for (const auto& colour : section.colours)
                addAndMakeVisible (rows.add (new ColourRow (colour, history)));
        }

        setSize (400, layOut());
    }

    void refresh()
    {
        borders.setToggleState (theme().boxBorders, juce::dontSendNotification);

        for (auto* row : rows)
            row->refresh();
    }

    void resized() override { layOut(); }

private:
    // returns the height it all takes
    int layOut()
    {
        auto y = 0;
        auto row = 0;

        for (int i = 0; i < (int) themeSections().size(); ++i)
        {
            const auto& section = themeSections()[(size_t) i];

            headings[i]->setBounds (0, y + 12, getWidth(), 30);
            y += 46;

            if (section.id == "other")
            {
                borders.setBounds (0, y, getWidth(), rowHeight);
                y += rowHeight;
            }

            for (int c = 0; c < (int) section.colours.size(); ++c, ++row)
            {
                rows[row]->setBounds (0, y, getWidth(), rowHeight);
                y += rowHeight;
            }
        }

        return y + 12;
    }

    static constexpr int rowHeight = 28;

    ThemeHistory& history;

    juce::OwnedArray<juce::Label> headings;
    juce::OwnedArray<ColourRow> rows;
    BoxToggle borders { "BOX BORDERS" };
};

class ThemeCustomiserContent : public juce::Component,
                               private juce::ChangeListener
{
public:
    explicit ThemeCustomiserContent (ThemeHistory& themeHistory) : history (themeHistory)
    {
        for (auto* field : { &nameField, &authorField })
        {
            field->setEditable (true);
            field->setColour (juce::Label::backgroundColourId, juce::Colour (0xff161616));
            field->onTextChange = [this]
            {
                themes->info.name = nameField.getText();
                themes->info.author = authorField.getText();
                themes->edited();
            };
            addAndMakeVisible (field);
        }

        nameLabel.attachToComponent (&nameField, true);
        authorLabel.attachToComponent (&authorField, true);

        save.setTooltip ("Saves as author - name in the themes folder, where it can be picked from the settings page");
        save.onClick = [this]
        {
            if (themes->info.name.trim().isEmpty())
                themes->info.name = "Untitled";

            themes->save();

            // saving isn't picking another theme, so the undo history stays
            shownId = themes->getSelectedId();
        };
        addAndMakeVisible (save);

        undo.onClick = [this] { history.undo(); };
        redo.onClick = [this] { history.redo(); };
        undo.setTooltip ("Command Z");
        redo.setTooltip ("Command Shift Z");
        addAndMakeVisible (undo);
        addAndMakeVisible (redo);

        openFolder.onClick = []
        {
            ThemeManager::folder().createDirectory();
            ThemeManager::folder().revealToUser();
        };
        addAndMakeVisible (openFolder);

        viewport.setViewedComponent (&list, false);
        viewport.setScrollBarsShown (true, false);
        addAndMakeVisible (viewport);

        themes->addChangeListener (this);
        refresh();

        setSize (460, 720);
    }

    ~ThemeCustomiserContent() override { themes->removeChangeListener (this); }

    void paint (juce::Graphics& g) override { g.fillAll (juce::Colours::black); }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (12);

        auto top = bounds.removeFromTop (28);
        openFolder.setBounds (top.removeFromRight (110));
        top.removeFromRight (6);
        save.setBounds (top.removeFromRight (70));
        top.removeFromRight (12);
        nameField.setBounds (top.withTrimmedLeft (60));

        bounds.removeFromTop (6);
        auto second = bounds.removeFromTop (28);
        redo.setBounds (second.removeFromRight (110));
        second.removeFromRight (6);
        undo.setBounds (second.removeFromRight (70));
        second.removeFromRight (12);
        authorField.setBounds (second.withTrimmedLeft (60));

        bounds.removeFromTop (6);
        viewport.setBounds (bounds);
        list.setSize (viewport.getMaximumVisibleWidth(), list.getHeight());
    }

private:
    // a different theme picked, or a colour changed from another window
    void changeListenerCallback (juce::ChangeBroadcaster*) override { refresh(); }

    void refresh()
    {
        // picking another theme starts its history over
        if (themes->getSelectedId() != ThemeManager::unsavedId && themes->getSelectedId() != shownId)
            history.clear();

        shownId = themes->getSelectedId();
        undo.setEnabled (history.canUndo());
        redo.setEnabled (history.canRedo());

        nameField.setText (themes->info.name, juce::dontSendNotification);
        authorField.setText (themes->info.author, juce::dontSendNotification);

        list.refresh();
    }

    ThemeHistory& history;
    juce::SharedResourcePointer<ThemeManager> themes;
    juce::String shownId;

    juce::Label nameField, authorField;
    juce::Label nameLabel { {}, "Name" }, authorLabel { {}, "Author" };
    juce::TextButton save { "SAVE" }, openFolder { "OPEN FOLDER" }, undo { "UNDO" }, redo { "REDO" };

    ColourList list { history };
    juce::Viewport viewport;
    juce::TooltipWindow tooltips { this, 500 };
};

class ThemeCustomiser : public juce::DocumentWindow
{
public:
    ThemeCustomiser() : juce::DocumentWindow ("Hamburger Theme", juce::Colours::black, closeButton)
    {
        setLookAndFeel (&lookAndFeel);
        setUsingNativeTitleBar (true);
        setContentOwned (new ThemeCustomiserContent (history), true);
        setResizable (true, false);
        setAlwaysOnTop (true);
        centreWithSize (getWidth(), getHeight());
    }

    // the content goes first, while the look and feel it's drawn with is still around
    ~ThemeCustomiser() override
    {
        clearContentComponent();
        setLookAndFeel (nullptr);
    }

    void closeButtonPressed() override { setVisible (false); }

    // typing in a field has its own undo, these are for everything else in the window
    bool keyPressed (const juce::KeyPress& key) override
    {
        const auto command = juce::ModifierKeys::commandModifier;

        if (key == juce::KeyPress ('z', command, 0))
            history.undo();
        else if (key == juce::KeyPress ('z', command | juce::ModifierKeys::shiftModifier, 0) || key == juce::KeyPress ('y', command, 0))
            history.redo();
        else
            return false;

        return true;
    }

private:
    CustomiserLookAndFeel lookAndFeel;
    juce::SharedResourcePointer<ThemeManager> themes;
    ThemeHistory history { *themes };
};
