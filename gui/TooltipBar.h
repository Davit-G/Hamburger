#pragma once

#include "LookAndFeel/Theme.h"
#include "Modules/Panel.h"

// a strip across the top of the start page saying what whatever's under the mouse does
class TooltipBar : public juce::Component
{
public:
    static constexpr int height = 32;

    TooltipBar()
    {
        setInterceptsMouseClicks (false, false);
        juce::Desktop::getInstance().addGlobalMouseListener (this);
    }

    ~TooltipBar() override { juce::Desktop::getInstance().removeGlobalMouseListener (this); }

    // hears the mouse moving over every window, so only takes tips from the one it's in. the last one stays up until another
    void mouseMove (const juce::MouseEvent& e) override
    {
        auto* hovered = juce::Desktop::getInstance().findComponentAt (e.getScreenPosition());

        if (hovered == nullptr || ! getTopLevelComponent()->isParentOf (hovered))
            return;

        auto* client = dynamic_cast<juce::TooltipClient*> (hovered);
        auto tip = client != nullptr ? client->getTooltip() : juce::String();

        if (auto* slider = dynamic_cast<juce::Slider*> (hovered); slider != nullptr && tip.isNotEmpty())
            tip = slider->getName() + ": " + tip;

        if (tip.isEmpty() || tip == text)
            return;

        text = tip;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const auto box = getLocalBounds().reduced (Panel::boxInset).toFloat();

        g.setColour (theme().box);
        g.fillRoundedRectangle (box, Panel::boxCornerSize);

        g.setFont (getLookAndFeel().getPopupMenuFont().withHeight (14.0f));
        g.setColour (theme().textInfo);
        g.drawFittedText (text, box.reduced (Panel::boxPadding, 0.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.9f);
    }

private:
    juce::String text;
};
