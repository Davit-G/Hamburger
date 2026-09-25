#pragma once

#include "../../PluginProcessor.h"
#include "LightButton.h"
#include "../Knob.h"
#include "Panel.h"
#include "SlotLevels.h"

class Module : public juce::Component
{
public:
    Module(AudioPluginAudioProcessor &processor, const std::string &moduleName, const std::string buttonAttachmentId, const std::string categoryAttachmentId, std::vector<std::unique_ptr<Panel>> panels, bool noHeader = false)
        : modulePanels(std::move(panels)),
          scopeContext(processor.getScopeContext())
    {
        this->noHeader = noHeader;

        if (!noHeader)
        {
            setupHeader();
            setupTitleLabel(moduleName);
            setupCategorySelector(moduleName);
        }

        this->moduleName = moduleName;

        if (auto* type = dynamic_cast<juce::AudioParameterChoice*>(processor.treeState.getParameter(categoryAttachmentId)))
        {
            // only the implemented types, the parameter's reserved slots after them stay out of the menu
            jassert(type->choices.size() >= (int) modulePanels.size());

            for (int i = 0; i < (int) modulePanels.size(); ++i)
                typeNames.add(type->choices[i]);
        }
        else
        {
            for (auto &panel : modulePanels)
                typeNames.add(panel->getName());
        }

        setupPanels();

        if (modulePanels.size() > 1)
            categorySelector.setTooltip("Click to choose between different effect types");

        setPaintingIsUnclipped(true);

        /*  By index, not a ComboBoxAttachment: that spreads the parameter's 0 to 1 over the menu's items, and the menu
            has fewer items than the parameter has choices. */
        if (auto* type = dynamic_cast<juce::AudioParameterChoice*>(processor.treeState.getParameter(categoryAttachmentId)))
        {
            categoryAttachment = std::make_unique<juce::ParameterAttachment>(*type, [this](float index)
            {
                categorySelector.setSelectedItemIndex((int) index, juce::dontSendNotification);
                setScreen((int) index);
                setCategoryText(this->moduleName);
                resized();
            }, nullptr);

            categoryAttachment->sendInitialUpdate();
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
            enabledButton = std::make_unique<LightButton>(powerGlyph(), &Theme::powerOn, &Theme::powerOff);
            enabledButton->setTooltip(powerTooltipFor(buttonAttachmentId, moduleName));
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
        
        for (auto &panel : modulePanels) {
            panel->addMouseListener(this, true);
        }
    }

    // void mouseUp(const juce::MouseEvent &event) override {
    //     scopeContext.setType(ScopeContextType::LR_SCOPE);
    // }

    // void mouseDown(const juce::MouseEvent &event) override {
    //     scopeContext.setType(ScopeContextType::IN_OUT);
    // }

    void hideElements()
    {
        if (enabledButton != nullptr || hamburgerEnabled != nullptr)
        {
            for (auto &panel : modulePanels)
            {
                panel->setAlpha(1.0f);
            }
        }

        if (enabledButton != nullptr && enabledButton->getToggleState())
        {
            for (auto &panel : modulePanels)
            {
                panel->setAlpha(1.0f);
            }
        }
        else if (enabledButton != nullptr)
        {
            for (auto &panel : modulePanels)
            {
                panel->setAlpha(0.6f);
            }
        }
    }

    ~Module() {
        for (auto &panel : modulePanels) {
            panel->removeMouseListener(this);
        }
    };

    void paint(juce::Graphics &g) override
    {
        const auto box = getLocalBounds().reduced(Panel::boxInset).toFloat();
        const auto on = enabledButton == nullptr || enabledButton->getToggleState();

        g.setColour(on ? theme().box : theme().boxDisabled);
        g.fillRoundedRectangle(box, Panel::boxCornerSize);

        if (theme().boxBorders)
        {
            g.setColour(on ? theme().boxBorder : theme().boxBorderDisabled);
            g.drawRoundedRectangle(box.reduced(0.5f), Panel::boxCornerSize, 1.0f);
        }
    }

    void lookAndFeelChanged() override { applyAccent(); }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(Panel::boxPadding);

        if (noHeader)
        {
            for (auto &panel : modulePanels)
            {
                panel->setBounds(bounds);
            }
            return;
        }

        juce::ignoreUnused (lowerContent);

        auto titleBounds = bounds.removeFromTop(30);
        titleBounds.reduce(10, 0);

        auto extraSpace = 0.0f;

        header.items.clear();
        if (enabledButton != nullptr)
        {
            header.items.add(juce::FlexItem(*enabledButton).withMinWidth(15.0f).withMargin(7.5f));
            extraSpace = 15.0f + 7.5f;
        }

        auto titleFont = titleLabel.getFont();

        if (modulePanels.size() == 1)
        {
            header.items.add(juce::FlexItem(titleLabel).withMinWidth(juce::GlyphArrangement::getStringWidth(titleFont, titleLabel.getText()) * 1.4f));
        }
        else
        {
            header.items.add(juce::FlexItem(categorySelector).withMinWidth(juce::GlyphArrangement::getStringWidth(titleFont, categorySelector.getText()) * 1.4f + extraSpace));
        }

        header.performLayout(titleBounds);

        const auto box = getLocalBounds().reduced(Panel::boxInset);

        auto placeFooterAtBottom = [&](juce::Rectangle<int>& area)
        {
            area.setBottom(box.getBottom() - footerBottomInset);
            footer->setBounds(area.removeFromBottom(footerHeight).withLeft(bounds.getX()).withRight(bounds.getRight())
                                  .expanded(0, footerHitMargin));
        };

        if (lowerContent != nullptr && lowerContent->isVisible() && lowerContent->getParentComponent() == this)
        {
            auto lower = bounds.removeFromBottom(bounds.getHeight() * lowerContentShare / 5).withLeft(box.getX()).withRight(box.getRight());

            if (lowerContentIsFlush)
                lower.setBottom(box.getBottom());
            else if (footer != nullptr)
                placeFooterAtBottom(lower);

            // flush content starts footerBottomInset under the strip, which sits at the bottom of the panels' area
            if (lowerContentIsFlush && footer != nullptr)
            {
                footer->setBounds(bounds.removeFromBottom(footerHeight).expanded(0, footerHitMargin));
                lower.setTop(footer->getBottom() + footerBottomInset);
            }

            lowerContent->setBounds(lower);
        }
        else if (footer != nullptr)
        {
            placeFooterAtBottom(bounds);
        }

        for (auto &panel : modulePanels)
        {
            panel->setBounds(bounds);
        }
    }

    // the colours of the panel on screen
    AccentColours Theme::* getAccent() const
    {
        for (auto &panel : modulePanels)
            if (panel->isVisible())
                return panel->getAccent();

        return &Theme::plain;
    }

    std::function<void()> onScreenChanged;

    void setFooter(std::unique_ptr<juce::Component> newFooter, int height)
    {
        footer = std::move(newFooter);
        footerHeight = height;
        addAndMakeVisible(*footer);
        applyAccent();
        resized();
    }

    // flush content runs to the box's bottom edge, with the footer moved up above it
    void setLowerContent(juce::Component* content, bool isFlush)
    {
        lowerContent = content;
        lowerContentIsFlush = isFlush;

        if (lowerContent != nullptr)
            addAndMakeVisible(*lowerContent);

        if (footer != nullptr)
            footer->toFront(false);

        resized();
    }

    std::unique_ptr<LightButton> enabledButton = nullptr;
    juce::ComboBox categorySelector;

private:
    // disabling isn't quite the same for every box
    static juce::String powerTooltipFor(const std::string &attachmentId, const std::string &moduleName)
    {
        if (attachmentId == ParamIDs::hamburgerEnabled.getParamID().toStdString())
            return "Enable / disable hamburger.";

        if (juce::String(moduleName).containsIgnoreCase("DISTORTION"))
            return "Enable / disable this distortion. Out gain still applies.";

        return "Enable / disable audio processing";
    }

    juce::Component* lowerContent = nullptr;
    bool lowerContentIsFlush = false;

    std::unique_ptr<juce::Component> footer;
    int footerHeight = 0;
    static constexpr int footerBottomInset = 4;
    static constexpr int lowerContentShare = 2;
    static constexpr int footerHitMargin = 4;
    
    // the heading and the levels under the panels follow whichever type is showing
    void applyAccent()
    {
        const auto& accent = theme().*getAccent();

        titleLabel.setColour(juce::Label::textColourId, accent.text);
        categorySelector.setColour(juce::ComboBox::textColourId, accent.text);
        categorySelector.setColour(juce::ComboBox::arrowColourId, accent.text);

        if (auto* levels = dynamic_cast<SlotLevels*>(footer.get()))
            levels->setAccent(getAccent());
    }

    void setupHeader()
    {
        header.flexDirection = juce::FlexBox::Direction::row;
        header.justifyContent = juce::FlexBox::JustifyContent::center;
    }

    // a type as the menu shows it, with the module's name after it where it has one
    juce::String typeLabel(int index) const
    {
        return moduleName.empty() ? typeNames[index] : typeNames[index] + " " + moduleName;
    }

    void setupPanels()
    {
        for (int i = 0; i < (int) modulePanels.size(); ++i)
        {
            categorySelector.addItem(typeLabel(i), i + 1);
            addAndMakeVisible(modulePanels[(size_t) i].get());
            modulePanels[(size_t) i]->setVisible(false);
        }
    }

    void setupTitleLabel(const std::string &moduleName)
    {
        titleLabel.setText(moduleName, juce::dontSendNotification);
        
        addAndMakeVisible(titleLabel);
        
    }

    void setCategoryText(juce::String moduleName)
    {
        int index = categorySelector.getSelectedItemIndex(); 
        
        if (index != -1 && index < (int) modulePanels.size())
        {
            categorySelector.setText(typeLabel(index), juce::dontSendNotification);
        }
        else
        {
            categorySelector.setText(moduleName, juce::dontSendNotification);
        }
    }

    void setupCategorySelector(const std::string &moduleName)
    {
        categorySelector.onChange = [this, moduleName]
        {
            auto selection = categorySelector.getSelectedItemIndex();
            if (selection != -1)
            {
                if (categoryAttachment != nullptr)
                    categoryAttachment->setValueAsCompleteGesture((float) selection);

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

            applyAccent();

            if (onScreenChanged != nullptr)
                onScreenChanged();
        }
    }

    ScopeContext& scopeContext;

    std::unique_ptr<juce::ParameterAttachment> categoryAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment = nullptr;

    juce::AudioParameterBool* hamburgerEnabled;
    
    // array of pointers of panel
    std::vector<std::unique_ptr<Panel>> modulePanels;
    juce::StringArray typeNames;
    std::string moduleName;

    juce::FlexBox header;
    juce::Label titleLabel;
    std::string moduleNameTitle;

    bool noHeader;
};