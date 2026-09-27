#pragma once

#include "../Panel.h"

#include "juce_gui_basics/juce_gui_basics.h"

#include "../Scope.h"

class LogoPanel : public Panel, public juce::SettableTooltipClient, private juce::Timer, public juce::ChangeListener
{
public:
    LogoPanel(AudioPluginAudioProcessor &p) : Panel(p, "SETTINGS"), processorRef(p)
    {
        helptext.setJustificationType(juce::Justification::topLeft);
        helptext.setFont(getLookAndFeel().getLabelFont(helptext));
        helptext.setMinimumHorizontalScale(0.9f);
        helptext.setInterceptsMouseClicks(false, false);
        addAndMakeVisible(helptext);

        displayHelpText = p.getAppProperties().getTooltipType() == AppProperties::TooltipType::boxLabel;

        setMouseCursor(juce::MouseCursor::PointingHandCursor);
        setTooltip("Click for info about Hamburger and the credits");

        juce::Desktop::getInstance().addGlobalMouseListener(&tooltipHelper);

        p.getAppProperties().addChangeListener(this);
    }

    void changeListenerCallback (juce::ChangeBroadcaster* source) override {
        auto appProperties = &processorRef.getAppProperties();
        if (source == appProperties) {
            displayHelpText = appProperties->getTooltipType() == AppProperties::TooltipType::boxLabel;
            
            if (!displayHelpText) {
                drawableLogoString->setVisible(true);
                helptext.setVisible(false);
            }
            repaint();
        }
    }

    void lookAndFeelChanged() override
    {
        Panel::lookAndFeelChanged();

        helptext.setColour(juce::Label::textColourId, theme().textInfo);

        if (drawableLogoString != nullptr && drawnTop == theme().logoTop && drawnBottom == theme().logoBottom)
            return;

        drawnTop = theme().logoTop;
        drawnBottom = theme().logoBottom;

        auto svg = juce::parseXML(juce::String::createStringFromData(BinaryData::HamBurgerText_svg, BinaryData::HamBurgerText_svgSize));
        applyThemeToSvg(*svg);

        // hidden while help text is showing in its place
        const auto logoShown = drawableLogoString == nullptr || drawableLogoString->isVisible();

        drawableLogoString = juce::Drawable::createFromSVG(*svg);
        addChildComponent(*drawableLogoString);
        drawableLogoString->setVisible(logoShown);
        resized();
    }

    void timerCallback() {
        if (displayHelpText) {
            drawableLogoString->setVisible(true);
            helptext.setVisible(false);
            repaint();
        }
    }

    ~LogoPanel() {
        juce::Desktop::getInstance().removeGlobalMouseListener(&tooltipHelper);
        processorRef.getAppProperties().removeChangeListener(this);
    }

    void mouseUp(const juce::MouseEvent &event) override
    {
        getParentComponent()->getParentComponent()->getParentComponent()->postCommandMessage(0);
    }

    void resized() override
    {
        // placing the drawing itself: fitting the component instead sticks at the smallest the panel has ever been, with the
        // drawing still its full size from the component's corner, so it hangs off to the bottom right
        if (drawableLogoString != nullptr && ! getLocalBounds().isEmpty())
            drawableLogoString->setTransformToFit(getLocalBounds().toFloat(),
                                                  juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
        helptext.setBounds(getLocalBounds());
    }
    
    void changeLabelText(juce::String txt) {
        if (displayHelpText) {
            helptext.setText(txt, juce::dontSendNotification);
            drawableLogoString->setVisible(false);
            helptext.setVisible(true);
            repaint();
    
            startTimer(10000);
        }
    }
private:
    AudioPluginAudioProcessor &processorRef;

    class TooltipListenerHelper : public juce::MouseListener
    {
        public:
        TooltipListenerHelper(LogoPanel &l) : lp(l) {}

        void mouseMove(const juce::MouseEvent& event) override
            {
                juce::Point<int> screenPos = event.getScreenPosition();

                auto* hoveredComponent = juce::Desktop::getInstance().findComponentAt(screenPos);
                
                if (auto* client = dynamic_cast<juce::TooltipClient*>(hoveredComponent))
                {
                    const auto tooltip = client->getTooltip();

                    if (tooltip.isEmpty() || tooltip == previousTooltip)
                        return;

                    previousTooltip = tooltip;

                    if (auto* slider = dynamic_cast<juce::Slider*>(hoveredComponent))
                        lp.changeLabelText(slider->getName() + ": " + tooltip);
                    else
                        lp.changeLabelText(tooltip);
                }
            }
        juce::String previousTooltip;

        LogoPanel &lp;
    };

    TooltipListenerHelper tooltipHelper {*this}; 

    juce::Label helptext;
    bool displayHelpText = false;

    std::unique_ptr<juce::Drawable> drawableLogoString = nullptr;
    juce::Colour drawnTop, drawnBottom;
};