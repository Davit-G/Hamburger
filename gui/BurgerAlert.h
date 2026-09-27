#pragma once

#include "juce_gui_basics/juce_gui_basics.h"
#include "./LookAndFeel/HamburgerLAF.h"
#include "./LookAndFeel/Theme.h"

class BurgerAlert : public juce::AlertWindow
{
public:
    BurgerAlert(const juce::String &title, const juce::String &message, juce::AlertWindow::AlertIconType iconType)
        : juce::AlertWindow(title, message, iconType)
    {
        setLookAndFeel(&comboBoxLook);

        setColour(juce::AlertWindow::backgroundColourId, theme().alertBackground);
        setColour(juce::AlertWindow::textColourId, theme().alertText);
        setColour(juce::AlertWindow::outlineColourId, theme().alertOutline);
        
        setColour(juce::Label::ColourIds::backgroundColourId, theme().box);
        setColour(juce::Label::ColourIds::textColourId, theme().alertText);
        setColour(juce::Label::ColourIds::outlineColourId, theme().alertText);

        // getTopLevelWindow(0)->setDropShadowEnabled(true);

        // auto okButton = getButton(0);
        // okButton->onClick = [this]() {
        //     // Force text editors to commit their content
        //     unfocusAllComponents();
        //     exitModalState(1);
        // };
    }

    ~BurgerAlert() {
        setLookAndFeel(nullptr);
    }

    void createPresetSaveAlert(juce::String defaultName, juce::String defaultAuthor, juce::String defaultDescription) {


        addTextEditor("presetName", defaultName, "Preset Name");
        addTextEditor("author", defaultAuthor, "Author");

        // several lines, so it's its own editor rather than one of the window's single line ones
        description.setMultiLine(true, true);
        description.setReturnKeyStartsNewLine(true);
        description.setText(defaultDescription, false);
        description.setTextToShowWhenEmpty("Description", theme().textPresetsAuthor);
        description.setColour(juce::TextEditor::backgroundColourId, theme().row);
        description.setColour(juce::TextEditor::textColourId, theme().alertText);
        description.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        description.setSize(300, 90);
        addCustomComponent(&description);

		addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
		addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));

        auto presetTextEditor = getTextEditor("presetName");
        auto authorTextEditor = getTextEditor("author");

        presetTextEditor->setColour(juce::TextEditor::backgroundColourId, theme().row);
        presetTextEditor->setColour(juce::TextEditor::ColourIds::outlineColourId, juce::Colours::transparentBlack);
        // presetTextEditor->setFont(comboBoxLook.getPopupMenuFont());
        authorTextEditor->setColour(juce::TextEditor::backgroundColourId, theme().row);
        authorTextEditor->setColour(juce::TextEditor::ColourIds::outlineColourId, juce::Colours::transparentBlack);
        // authorTextEditor->setFont(comboBoxLook.getPopupMenuFont());

        auto okButton = getButton(0);
        auto cancelButton = getButton(1);

        okButton->setColour(juce::TextButton::buttonColourId, theme().row);
        okButton->setColour(juce::TextButton::textColourOnId, theme().alertText);
        okButton->setColour(juce::TextButton::textColourOffId, theme().alertText);

        cancelButton->setColour(juce::TextButton::buttonColourId, theme().row);
        cancelButton->setColour(juce::TextButton::textColourOnId, theme().alertText);
        cancelButton->setColour(juce::TextButton::textColourOffId, theme().alertText);
    }

    juce::String getDescription() const { return description.getText(); }

    void createPresetRenameAlert(juce::String currentName) {
        addTextEditor("presetName", currentName, "New Name");
        addButton("Rename", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
        addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));

        getTextEditor("presetName")->setColour(juce::TextEditor::backgroundColourId, theme().row);
        getTextEditor("presetName")->setColour(juce::TextEditor::ColourIds::outlineColourId, juce::Colours::transparentBlack);

        for (int i = 0; i < 2; ++i)
        {
            getButton(i)->setColour(juce::TextButton::buttonColourId, theme().row);
            getButton(i)->setColour(juce::TextButton::textColourOnId, theme().alertText);
            getButton(i)->setColour(juce::TextButton::textColourOffId, theme().alertText);
        }
    }

    void createPresetWarning() {
        addButton("Ok", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));

        auto okButton = getButton(0);

        // todo: repaint alert window to make this look better in general idk :shrug:

        okButton->setColour(juce::TextButton::buttonColourId, theme().row);
        okButton->setColour(juce::TextButton::textColourOnId, theme().alertText);
        okButton->setColour(juce::TextButton::textColourOffId, theme().alertText);
    }

    void createPresetDeleteAlert() {
        addButton("Delete", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
        addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));

        auto deleteButton = getButton(0);
        auto cancelButton = getButton(1);

        // todo: repaint alert window to make this look better in general idk :shrug:

        deleteButton->setColour(juce::TextButton::buttonColourId, theme().alertAccent);
        deleteButton->setColour(juce::TextButton::textColourOnId, theme().alertAccentText);
        deleteButton->setColour(juce::TextButton::textColourOffId, theme().alertAccentText);

        cancelButton->setColour(juce::TextButton::buttonColourId, theme().row);
        cancelButton->setColour(juce::TextButton::textColourOnId, theme().alertText);
        cancelButton->setColour(juce::TextButton::textColourOffId, theme().alertText);
    }

    void createUpdateAlert(const juce::String& releaseNotes) {
        addButton("Update", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
        addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));
        addButton("Don't show again", 2);

        auto updateButton = getButton(0);
        auto cancelButton = getButton(1);
        auto dontShowButton = getButton(2);

        updateButton->setColour(juce::TextButton::buttonColourId, theme().alertAccent);
        updateButton->setColour(juce::TextButton::textColourOnId, theme().alertAccentText);
        updateButton->setColour(juce::TextButton::textColourOffId, theme().alertAccentText);

        cancelButton->setColour(juce::TextButton::buttonColourId, theme().row);
        cancelButton->setColour(juce::TextButton::textColourOnId, theme().alertText);
        cancelButton->setColour(juce::TextButton::textColourOffId, theme().alertText);

        dontShowButton->setColour(juce::TextButton::buttonColourId, theme().row);
        dontShowButton->setColour(juce::TextButton::textColourOnId, theme().alertText);
        dontShowButton->setColour(juce::TextButton::textColourOffId, theme().alertText);

        if (releaseNotes.isNotEmpty())
        {
            auto* scrollBox = new juce::TextEditor();
            scrollBox->setMultiLine(true, true);
            scrollBox->setReadOnly(true);
            scrollBox->setScrollbarsShown(true);
            scrollBox->setCaretVisible(false);
            scrollBox->setPopupMenuEnabled(false);
            scrollBox->setText(releaseNotes, false);
            
            scrollBox->setMouseClickGrabsKeyboardFocus(false);
            scrollBox->setWantsKeyboardFocus(false);

            scrollBox->setColour(juce::TextEditor::backgroundColourId, theme().row);
            scrollBox->setColour(juce::TextEditor::textColourId, theme().alertText);
            scrollBox->setColour(juce::TextEditor::outlineColourId, theme().alertOutline);
            
            scrollBox->setSize(400, 160); 
            addCustomComponent(scrollBox);
        }
    }

private:

    HamburgerLAF comboBoxLook;
    juce::TextEditor description;
};