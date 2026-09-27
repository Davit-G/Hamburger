#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

#include "BurgerAlert.h"
#include "PluginHeader.h"
#include "PluginFooter.h"
#include "PresetBrowser.h"
#include "LookAndFeel/Theme.h"

const std::function<void(std::string)> errorAlertCallback = [](std::string result)
{
	DBG(result);
	auto errorAlert = new BurgerAlert("Error", result, juce::AlertWindow::AlertIconType::WarningIcon);

	errorAlert->createPresetWarning();
	errorAlert->enterModalState(true, nullptr, true);
};

// the preset name in the header with the save button and the arrows beside it, and the preset view under the header when it's open
class PresetPanel : public juce::Component
{
public:
	PresetPanel(Preset::PresetManager &pm) : presetManager(pm), browser(pm)
	{
		for (auto *button : {&saveButton, &previousPresetButton, &nextPresetButton, &closeButton})
			addAndMakeVisible(button);

		closeButton.setVisible(false);
		addChildComponent(browser);

		saveButton.onClick = [this] { savePreset(); };
		browser.saveButton.onClick = [this] { savePreset(); };
		browser.renameButton.onClick = [this] { renamePreset(); };
		browser.deleteButton.onClick = [this] { deletePreset(); };
		browser.folderButton.onClick = [this] { presetManager.getPresetDirectory().startAsProcess(); };

		previousPresetButton.onClick = [this] { showLoaded(presetManager.loadPreviousPreset(errorAlertCallback)); };
		nextPresetButton.onClick = [this] { showLoaded(presetManager.loadNextPreset(errorAlertCallback)); };

		browser.onPresetClicked = [this](const juce::File &preset)
		{
			presetManager.loadPreset(preset, errorAlertCallback);
			showLoaded(preset);
		};
		browser.onPresetDoubleClicked = [this](const juce::File &) { setBrowserOpen(false); };

		closeButton.onClick = [this] { setBrowserOpen(false); };

		currentPresetLabel.setMouseCursor(juce::MouseCursor::PointingHandCursor);
		currentPresetLabel.onClick = [this] { setBrowserOpen(!browser.isVisible()); };
		addAndMakeVisible(currentPresetLabel);

		// only the controls take clicks, the rest passes through to whatever's under the header
		setInterceptsMouseClicks(false, true);

		const auto current = presetManager.getCurrentPreset();
		currentPresetLabel.setButtonText(current.existsAsFile() ? labelFor(current) : juce::String("Hamburger"));
	}

	void lookAndFeelChanged() override
	{
		currentPresetLabel.setColour(juce::TextButton::ColourIds::buttonColourId, theme().box);
		currentPresetLabel.setColour(juce::TextButton::ColourIds::textColourOffId, theme().textPresets);
	}

	// the header's tabs on the right, which the preset controls stay clear of
	void setReservedRight(int width)
	{
		reservedRight = width;
		resized();
	}

	void resized() override
	{
		// lined up with the header's bar
		const auto height = PluginHeader::totalHeight;
		auto bounds = getLocalBounds().withTrimmedRight(reservedRight).removeFromTop(height - PluginHeader::pillInset).reduced(4, 0).withTrimmedLeft(4).withTrimmedRight(4);

		nextPresetButton.setBounds(bounds.removeFromRight(switcherWidth).reduced(4));
		previousPresetButton.setBounds(bounds.removeFromRight(switcherWidth).reduced(4));
		saveButton.setBounds(bounds.removeFromRight(switcherWidth).reduced(4));
		currentPresetLabel.setBounds(bounds.removeFromRight(presetNameWidth).reduced(4));
		closeButton.setBounds(bounds.removeFromRight(height).reduced(8));

		// the whole region between the header and the footer
		browser.setBounds(getLocalBounds().withTrimmedTop(height).withTrimmedBottom(PluginFooter::totalHeight));
	}

private:
	static juce::String labelFor(const juce::File &preset)
	{
		return preset.getParentDirectory().getFileName() + " - " + preset.getFileNameWithoutExtension();
	}

	void setBrowserOpen(bool open)
	{
		if (open)
			browser.refresh();

		browser.setVisible(open);
		closeButton.setVisible(open);
	}

	void showLoaded(const juce::File &preset)
	{
		currentPresetLabel.setButtonText(labelFor(preset));

		if (browser.isVisible())
			browser.refresh();
	}

	void savePreset()
	{
		auto alertWindow = new BurgerAlert("Save Preset", "Enter a name for your new preset: ", juce::MessageBoxIconType::NoIcon);

		auto defaultAuthor = presetManager.getCurrentAuthor();
		if (defaultAuthor.isEmpty())
			defaultAuthor = presetManager.getLastAuthor();
		alertWindow->createPresetSaveAlert(presetManager.getCurrentPresetName(), defaultAuthor, presetManager.getCurrentDescription());

		alertWindow->enterModalState(true, juce::ModalCallbackFunction::create([this, alertWindow](int result)
		{
			if (result == 1)
			{
				// commits whatever's still being typed
				alertWindow->unfocusAllComponents();

				const auto name = alertWindow->getTextEditor("presetName")->getText();

				if (presetManager.savePreset(name, alertWindow->getTextEditor("author")->getText(), alertWindow->getDescription(), errorAlertCallback))
					showLoaded(presetManager.getCurrentPreset());
			}

			delete alertWindow;
		}));
	}

	void renamePreset()
	{
		const auto preset = browser.getSelected();

		if (!preset.existsAsFile())
			return;

		auto alertWindow = new BurgerAlert("Rename Preset", "Enter a new name for " + preset.getFileNameWithoutExtension() + ": ", juce::MessageBoxIconType::NoIcon);
		alertWindow->createPresetRenameAlert(preset.getFileNameWithoutExtension());

		alertWindow->enterModalState(true, juce::ModalCallbackFunction::create([this, alertWindow, preset](int result)
		{
			if (result == 1 && presetManager.renamePreset(preset, alertWindow->getTextEditor("presetName")->getText(), errorAlertCallback))
			{
				browser.refresh();

				if (presetManager.getCurrentPreset().existsAsFile())
					currentPresetLabel.setButtonText(labelFor(presetManager.getCurrentPreset()));
			}

			delete alertWindow;
		}));
	}

	void deletePreset()
	{
		const auto preset = browser.getSelected();

		if (!preset.existsAsFile())
			return;

		auto alertWindow = new BurgerAlert("Delete Preset", "Are you sure you want to delete " + preset.getFileNameWithoutExtension() + "? ", juce::MessageBoxIconType::NoIcon);
		alertWindow->createPresetDeleteAlert();

		alertWindow->enterModalState(true, juce::ModalCallbackFunction::create([this, alertWindow, preset](int result)
		{
			if (result == 1)
			{
				presetManager.deletePreset(preset, errorAlertCallback);
				browser.refresh();
			}

			delete alertWindow;
		}));
	}

	static constexpr int switcherWidth = 24;
	static constexpr int presetNameWidth = 170;

	Preset::PresetManager &presetManager;

	IconButton saveButton{"Save the current sound as a preset", PresetIcons::save},
		previousPresetButton{"Previous preset", PresetIcons::previous},
		nextPresetButton{"Next preset", PresetIcons::next},
		closeButton{"Close the presets", PresetIcons::close};

	juce::TextButton currentPresetLabel;
	PresetBrowser browser;
	int reservedRight = 0;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetPanel)
};
