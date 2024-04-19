#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

#include "LookAndFeel/ComboBoxLookAndFeel.h"

class BurgerAlert : public juce::AlertWindow
{
public:
    BurgerAlert(const juce::String &title, const juce::String &message, juce::AlertWindow::AlertIconType iconType)
        : juce::AlertWindow(title, message, iconType)
    {
        setLookAndFeel(&comboBoxLook);

        setColour(juce::AlertWindow::backgroundColourId, juce::Colour::fromRGB(11, 11, 11));
        setColour(juce::AlertWindow::textColourId, juce::Colours::white);
        setColour(juce::AlertWindow::outlineColourId, juce::Colour::fromRGB(22, 22, 22));
        
        setColour(juce::Label::ColourIds::backgroundColourId, juce::Colours::black);
        setColour(juce::Label::ColourIds::textColourId, juce::Colours::white);
        setColour(juce::Label::ColourIds::outlineColourId, juce::Colours::white);

        getTopLevelWindow(0)->setDropShadowEnabled(true);
    }

    void createPresetSaveAlert() {
        addTextEditor("presetName", "MyBurger", "Preset Name");
        addTextEditor("author", "me", "Author");

		addButton("Save", 1, KeyPress(KeyPress::returnKey, 0, 0));
		addButton("Cancel", 0, KeyPress(KeyPress::escapeKey, 0, 0));

        auto presetTextEditor = getTextEditor("presetName");
        auto authorTextEditor = getTextEditor("author");

        presetTextEditor->setColour(juce::TextEditor::backgroundColourId, juce::Colour::fromRGB(22, 22, 22));
        presetTextEditor->setColour(juce::TextEditor::ColourIds::outlineColourId, juce::Colours::transparentBlack);
        presetTextEditor->setFont(comboBoxLook.getComboBoxFont());
        authorTextEditor->setColour(juce::TextEditor::backgroundColourId, juce::Colour::fromRGB(22, 22, 22));
        authorTextEditor->setColour(juce::TextEditor::ColourIds::outlineColourId, juce::Colours::transparentBlack);
        authorTextEditor->setFont(comboBoxLook.getComboBoxFont());

        auto okButton = getButton(0);
        auto cancelButton = getButton(1);

        okButton->setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(22, 22, 22));
        okButton->setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        okButton->setColour(juce::TextButton::textColourOffId, juce::Colours::white);

        cancelButton->setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(22, 22, 22));
        cancelButton->setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        cancelButton->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    }

    void createPresetWarning() {
        addButton("Ok", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));

        auto okButton = getButton(0);

        // todo: repaint alert window to make this look better in general idk :shrug:

        okButton->setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(22, 22, 22));
        okButton->setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        okButton->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    }

    void createPresetDeleteAlert() {
        addButton("Delete", 1, KeyPress(KeyPress::returnKey, 0, 0));
        addButton("Cancel", 0, KeyPress(KeyPress::escapeKey, 0, 0));

        auto deleteButton = getButton(0);
        auto cancelButton = getButton(1);

        // todo: repaint alert window to make this look better in general idk :shrug:

        deleteButton->setColour(juce::TextButton::buttonColourId, Palette::colours[0]);
        deleteButton->setColour(juce::TextButton::textColourOnId, juce::Colours::black);
        deleteButton->setColour(juce::TextButton::textColourOffId, juce::Colours::black);

        cancelButton->setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(22, 22, 22));
        cancelButton->setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        cancelButton->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    }

private:

    ComboBoxLookAndFeel comboBoxLook;
};