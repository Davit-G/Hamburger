#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

#include "LookAndFeel/ComboBoxLookAndFeel.h"

class BurgerSaveAlert : public juce::AlertWindow
{
public:
    BurgerSaveAlert(const juce::String &title, const juce::String &message, juce::AlertWindow::AlertIconType iconType)
        : juce::AlertWindow(title, message, iconType)
    {
        setLookAndFeel(&comboBoxLook);

        setColour(juce::AlertWindow::backgroundColourId, juce::Colour::fromRGB(11, 11, 11));
        setColour(juce::AlertWindow::textColourId, juce::Colours::white);
        setColour(juce::AlertWindow::outlineColourId, juce::Colour::fromRGB(22, 22, 22));
        
        setColour(juce::Label::ColourIds::backgroundColourId, juce::Colours::black);
        setColour(juce::Label::ColourIds::textColourId, juce::Colours::white);
        setColour(juce::Label::ColourIds::outlineColourId, juce::Colours::white);

        addTextEditor("presetName", "MyBurger", "Preset Name");
        addTextEditor("author", "me", "Author");
        addTextEditor("description", "", "Description");
		addButton("Save", 1, KeyPress(KeyPress::returnKey, 0, 0));
		addButton("Cancel", 0, KeyPress(KeyPress::escapeKey, 0, 0));

        auto presetTextEditor = getTextEditor("presetName");
        auto authorTextEditor = getTextEditor("author");
        auto descriptionTextEditor = getTextEditor("description");

        presetTextEditor->setColour(juce::TextEditor::backgroundColourId, juce::Colour::fromRGB(22, 22, 22));
        presetTextEditor->setColour(juce::TextEditor::ColourIds::outlineColourId, juce::Colours::transparentBlack);
        presetTextEditor->setFont(comboBoxLook.getComboBoxFont());
        authorTextEditor->setColour(juce::TextEditor::backgroundColourId, juce::Colour::fromRGB(22, 22, 22));
        authorTextEditor->setColour(juce::TextEditor::ColourIds::outlineColourId, juce::Colours::transparentBlack);
        authorTextEditor->setFont(comboBoxLook.getComboBoxFont());
        descriptionTextEditor->setColour(juce::TextEditor::backgroundColourId, juce::Colour::fromRGB(22, 22, 22));
        descriptionTextEditor->setColour(juce::TextEditor::ColourIds::outlineColourId, juce::Colours::transparentBlack);
        descriptionTextEditor->setFont(comboBoxLook.getComboBoxFont());

        auto okButton = getButton(0);
        auto cancelButton = getButton(1);

        // todo: repaint alert window to make this look better in general idk :shrug:

        okButton->setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(22, 22, 22));
        okButton->setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        okButton->setColour(juce::TextButton::textColourOffId, juce::Colours::white);

        cancelButton->setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(22, 22, 22));
        cancelButton->setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        cancelButton->setColour(juce::TextButton::textColourOffId, juce::Colours::white);

        getTopLevelWindow(0)->setDropShadowEnabled(true);
    }

private:

    ComboBoxLookAndFeel comboBoxLook;
};