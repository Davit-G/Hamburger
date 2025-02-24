#pragma once

#include "../../PluginProcessor.h"
#include "LightButton.h"
#include "../Knob.h"
#include "Panel.h"

class Module : public juce::Component
{
public:
    Module(AudioPluginAudioProcessor &processor, const std::string &moduleName, const std::string buttonAttachmentId, const std::string categoryAttachmentId, std::vector<std::unique_ptr<Panel>> panels, bool noHeader = false)
        : modulePanels(std::move(panels)),
          powerOffImage(juce::ImageCache::getFromMemory(BinaryData::poweroff_png, BinaryData::poweroff_pngSize)), // yes it's using imageCache but stop constructing new instances! unify somehow
          powerOnImage(juce::ImageCache::getFromMemory(BinaryData::poweron_png, BinaryData::poweron_pngSize))
    {
        this->noHeader = noHeader;

        if (!noHeader)
        {
            setupHeader();
            setupTitleLabel(moduleName);
            setupCategorySelector(moduleName);
        }

        setupPanels(moduleName);

        setPaintingIsUnclipped(true);

        if (categoryAttachmentId.length() > 0)
        {
            // on some linux based OSes, the category selector element overrides the parameter value, causing it to reset to default when opening the GUI.
            // the below two lines of code fixes that.
            auto existingCategoryParam = dynamic_cast<juce::AudioParameterChoice *>(processor.treeState.getParameter(categoryAttachmentId));
            categorySelector.setSelectedItemIndex(existingCategoryParam->getIndex()); 
            
            categoryAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.treeState, categoryAttachmentId, categorySelector);
            setScreen(categorySelector.getSelectedItemIndex());
        }
        else
        {
            setScreen(0);
            categorySelector.setSelectedItemIndex(0);
        }

        
        hamburgerEnabled = dynamic_cast<juce::AudioParameterBool *>(processor.treeState.getParameter(ParamIDs::hamburgerEnabled.getParamID()));
        jassert(hamburgerEnabled);

        if (buttonAttachmentId.length() > 0)
        {
            enabledButton = std::make_unique<LightButton>(processor, powerOffImage, powerOnImage);
            attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.treeState, buttonAttachmentId, *enabledButton);
            addAndMakeVisible(enabledButton.get());

            enabledButton->onClick = [this]
            {
                hideElements();
                repaint();
            };

            hideElements();
        }
        else
        {
            enabledButton = nullptr;
        }

        setCategoryText(moduleName);
        
    }

    void hideElements()
    {
        if (enabledButton != nullptr || hamburgerEnabled != nullptr)
        {
            for (auto &panel : modulePanels)
            {
                panel->setAlpha(1.0f);
            }
        }

        if (enabledButton->getToggleState())
            {

                for (auto &panel : modulePanels)
                {
                    panel->setAlpha(1.0f);
                }
            } else {

                for (auto &panel : modulePanels)
                {
                    panel->setAlpha(0.6f);
                }
            }
    }

    ~Module() override = default;

    void paint(juce::Graphics &g) override
    {
        Path p;
        p.addRoundedRectangle(getLocalBounds().reduced(4).toFloat(), 15.0f);

        if (enabledButton == nullptr) {
            g.setColour(juce::Colour::fromRGB(0, 0, 0));
        } else {
            if (enabledButton->getToggleState()) {
                g.setColour(juce::Colour::fromRGBA(0, 0, 0, 255));
            } else {
                g.setColour(juce::Colour::fromRGBA(0, 0, 0, 150));
            }
        }

        g.fillPath(p);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(12);

        if (noHeader)
        {
            for (auto &panel : modulePanels)
            {
                panel->setBounds(bounds);
            }
            return;
        }

        auto titleBounds = bounds.removeFromTop(30);
        titleBounds.reduce(10, 0);

        auto extraSpace = 0.0f;

        header.items.clear();
        if (enabledButton != nullptr)
        {
            header.items.add(FlexItem(*enabledButton).withMinWidth(15.0f).withMargin(7.5f));
            extraSpace = 15.0f + 7.5f;
        }

        auto titleFont = titleLabel.getFont();

        if (modulePanels.size() == 1)
        {
            header.items.add(FlexItem(titleLabel).withMinWidth(titleFont.getStringWidth(titleLabel.getText()) * 1.1f));
        }
        else
        {
            header.items.add(FlexItem(categorySelector).withMinWidth(titleFont.getStringWidth(categorySelector.getText()) * 1.4f + extraSpace));
        }

        header.performLayout(titleBounds);

        for (auto &panel : modulePanels)
        {
            panel->setBounds(bounds);
        }
    }

    std::unique_ptr<LightButton> enabledButton = nullptr;
    juce::ComboBox categorySelector;

private:
    void setupHeader()
    {
        header.flexDirection = juce::FlexBox::Direction::row;
        header.justifyContent = juce::FlexBox::JustifyContent::center;
    }

    void setupPanels(const std::string &moduleName)
    {
        int i = 1;
        for (auto &panel : modulePanels)
        {
            categorySelector.addItem(panel->getName() + " " + moduleName, i++);
            addAndMakeVisible(panel.get());
            panel->setVisible(false);
        }
    }

    void setupTitleLabel(const std::string &moduleName)
    {
        titleLabel.setText(moduleName, juce::dontSendNotification);
        titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        
        addAndMakeVisible(titleLabel);
        
    }

    void setCategoryText(juce::String moduleName)
    {
        size_t index = categorySelector.getSelectedItemIndex();
        if (index != -1)
        {
            auto panelName = modulePanels[index]->getName();
            if (moduleName == "") {
                categorySelector.setText(panelName);
            } else {
                categorySelector.setText(panelName + " " + moduleName);
            }
        }
        else
        {
            categorySelector.setText(moduleName);
        }
    }

    void setupCategorySelector(const std::string &moduleName)
    {
        categorySelector.onChange = [this, moduleName]
        {
            auto selection = categorySelector.getSelectedItemIndex();
            if (selection != -1)
            {
                setScreen(selection);
                this->resized();
            }
        };

        if (modulePanels.size() > 1)
        {
            setCategoryText(moduleName);
            addAndMakeVisible(categorySelector);
        }
    }

    void setScreen(int index)
    {
        if (index >= 0 && index < modulePanels.size())
        {
            for (auto &panel : modulePanels)
            {
                panel->setVisible(false);
            }
            modulePanels[index]->setVisible(true);
        }
    }

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> categoryAttachment = nullptr;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment = nullptr;

    juce::AudioParameterBool* hamburgerEnabled;
    
    // array of pointers of panel
    std::vector<std::unique_ptr<Panel>> modulePanels;

    juce::FlexBox header;
    juce::Label titleLabel;
    std::string moduleNameTitle;

    bool noHeader;

    juce::Image powerOffImage;
    juce::Image powerOnImage;
};