#pragma once

#include "../PluginProcessor.h"
#include "LookAndFeel/HamburgerLAF.h"
#include "FxOrderList.h"

// the settings tab, across everything under the header
class SettingsPage : public juce::Component
{
public:
    explicit SettingsPage(AudioPluginAudioProcessor& p) : processorRef(p), fxOrder(p)
    {
        for(auto* label : { &title, &orderTitle, &tooltipTitle, &presetFolderTitle })
        {
            label->setJustificationType(juce::Justification::centredLeft);
            addAndMakeVisible(label);
        }

        title.setText("SETTINGS", juce::dontSendNotification);
        HamburgerLAF::setLabelFontScale(title, 1.4f);

        orderTitle.setText("FX ORDER", juce::dontSendNotification);
        HamburgerLAF::setLabelFontScale(orderTitle, 1.4f);

        tooltipTitle.setText("TOOLTIPS", juce::dontSendNotification);
        presetFolderTitle.setText("PRESET FOLDER", juce::dontSendNotification);

        tooltipType.addItem("None", 1);
        tooltipType.addItem("Hovering", 2);
        tooltipType.addItem("Replace Logo", 3);
        tooltipType.setSelectedId(static_cast<int>(processorRef.getAppProperties().getTooltipType()) + 1, juce::dontSendNotification);
        tooltipType.onChange = [this] {
            processorRef.getAppProperties().setTooltipType(static_cast<AppProperties::TooltipType>(tooltipType.getSelectedId() - 1));
        };
        addAndMakeVisible(tooltipType);

        changePresetFolder.setColour(juce::TextButton::buttonColourId, juce::Colours::black);
        changePresetFolder.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        changePresetFolder.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        changePresetFolder.onClick = [this] {
            presetFolderChooser = std::make_unique<juce::FileChooser>("Select a folder to save presets to", Preset::defaultDirectory, "*.*", true);

            const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories;

            presetFolderChooser->launchAsync(flags, [this](const juce::FileChooser& chooser) {
                const auto directory = chooser.getResult();

                if(! directory.exists())
                    return;

                processorRef.getAppProperties().appProperties.getUserSettings()->setValue("presetFolder", directory.getFullPathName());
                processorRef.getPresetManager().setPresetDirectory(directory);
            });
        };
        addAndMakeVisible(changePresetFolder);

        addAndMakeVisible(fxOrder);
    }

    FxOrderList& getFxOrder() { return fxOrder; }

    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colours::black);
        g.fillRoundedRectangle(getLocalBounds().reduced(4).toFloat(), 15.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(20);

        // three columns across: the fx order under its title, then the general settings under theirs
        const auto columnWidth =(bounds.getWidth() - columnGap * 2) / 3;
        auto orderColumn = bounds.removeFromLeft(columnWidth);
        bounds.removeFromLeft(columnGap);

        orderTitle.setBounds(orderColumn.removeFromTop(30));
        orderColumn.removeFromTop(8);

        title.setBounds(bounds.removeFromTop(30));
        bounds.removeFromTop(8);

        auto general = bounds.removeFromLeft(columnWidth);

        for(auto [label, control] : std::initializer_list<std::pair<juce::Label*, juce::Component*>> {
                 { &tooltipTitle, &tooltipType }, { &presetFolderTitle, &changePresetFolder } })
        {
            auto row = general.removeFromTop(rowHeight);
            label->setBounds(row.removeFromLeft(row.getWidth() * 2 / 5));
            control->setBounds(row);
            general.removeFromTop(6);
        }

        fxOrder.setBounds(orderColumn);
    }

private:
    static constexpr int rowHeight = 24;
    static constexpr int columnGap = 24;

    AudioPluginAudioProcessor& processorRef;

    juce::Label title, orderTitle, tooltipTitle, presetFolderTitle;
    juce::ComboBox tooltipType;
    juce::TextButton changePresetFolder { "CHANGE FOLDER" };
    std::unique_ptr<juce::FileChooser> presetFolderChooser;

    FxOrderList fxOrder;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsPage)
};
