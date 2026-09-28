#pragma once

#include "RectSlider.h"
#include "Modules/Panel.h"
#include "Modules/LightButton.h"
#include "LookAndFeel/HamburgerLAF.h"

static_assert (ModSources::count == std::tuple_size_v<decltype (Theme::modSources)>, "a theme colour for every mod source");

inline juce::Font modFont (juce::Component& component, float height)
{
    if (auto* laf = dynamic_cast<HamburgerLAF*> (&component.getLookAndFeel()))
        return laf->getQuicksandFont().withHeight (height);

    return juce::Font (juce::FontOptions (height));
}

// a box like the others, without a power button or type of its own
class ModulationArea : public juce::Component
{
public:
    void paint (juce::Graphics& g) override
    {
        const auto box = getLocalBounds().reduced (Panel::boxInset).toFloat();

        g.setColour (theme().box);
        g.fillRoundedRectangle (box, Panel::boxCornerSize);

        if (theme().boxBorders)
        {
            g.setColour (theme().boxBorder);
            g.drawRoundedRectangle (box.reduced (0.5f), Panel::boxCornerSize, 1.0f);
        }
    }
};

/*  A source to drag onto any knob or slider, routing it there. Clicked rather than dragged, it's selected. Named, it
    shows the source in its colour, otherwise it's only the grip. */
class ModSourceButton : public juce::Component, public juce::SettableTooltipClient
{
public:
    ModSourceButton (const ModMatrix& m, int sourceIndex, bool showName) : matrix (m), source (sourceIndex), named (showName)
    {
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        setTooltip ("Drag onto a knob or slider to modulate it");
    }

    std::function<void()> onClick;

    void setSelected (bool shouldBeSelected)
    {
        selected = shouldBeSelected;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (1.0f);
        const auto colour = theme().modSources[(size_t) source];

        g.setColour (selected ? theme().rowSelected : theme().row);
        g.fillRoundedRectangle (bounds, 3.0f);

        // two columns of three dots
        auto grip = bounds.removeFromRight (12.0f).withSizeKeepingCentre (6.0f, 10.0f);
        g.setColour (colour);

        for (int column = 0; column < 2; ++column)
            for (int row = 0; row < 3; ++row)
                g.fillEllipse (grip.getX() + (float) column * 4.0f, grip.getY() + (float) row * 4.0f, 2.0f, 2.0f);

        if (named)
        {
            g.setFont (modFont (*this, 13.0f));
            g.drawText (matrix.sourceName (source), bounds.withTrimmedLeft (6.0f), juce::Justification::centredLeft, true);
        }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging || e.getDistanceFromDragStart() < 4)
            return;

        if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor (this))
        {
            dragging = true;
            container->startDragging (GenericKnob::modDragPrefix + juce::String (source), this);
        }
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (! dragging && onClick != nullptr)
            onClick();

        dragging = false;
    }

private:
    const ModMatrix& matrix;
    const int source;
    const bool named;
    bool selected = false, dragging = false;
};

/*  Under the distortion box: a scrolling list down the left of the drive and the four macros, each with a grip to drag
    it from, then the five modulators, and the selected modulator's settings taking the rest */
class ModulationStrip : public ModulationArea
{
public:
    explicit ModulationStrip (AudioPluginAudioProcessor& p)
    {
        viewport.setViewedComponent (&list, false);
        viewport.setScrollBarsShown (true, false);
        viewport.setScrollBarThickness (6);
        addAndMakeVisible (viewport);

        macros[0] = std::make_unique<RectSlider> (p, "DRIVE", ParamIDs::globalDrive);

        for (int i = 0; i < ParamIDs::numGlobalMacros; ++i)
            macros[(size_t) i + 1] = std::make_unique<RectSlider> (p, "MACRO " + juce::String (i + 1), *ParamIDs::globalMacros[i]);

        for (size_t i = 0; i < macros.size(); ++i)
        {
            macroGrips[i] = std::make_unique<ModSourceButton> (p.getModMatrix(), ModSources::drive + (int) i, false);

            // the wheel scrolls the list rather than turning whichever macro it's over
            macros[i]->setScrollWheelEnabled (false);
            list.addAndMakeVisible (*macros[i]);
            list.addAndMakeVisible (*macroGrips[i]);
        }

        for (int m = 0; m < ParamIDs::numModulators; ++m)
            modulators[(size_t) m] = std::make_unique<Modulator> (p, m, *this);

        select (0);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (Panel::boxInset + 8, Panel::boxInset + 6);

        viewport.setBounds (bounds.removeFromLeft (listWidth));
        bounds.removeFromLeft (12);

        const auto rows = (int) (macros.size() + modulators.size());
        list.setSize (viewport.getWidth() - viewport.getScrollBarThickness() - 2, rows * rowHeight);

        auto rowArea = list.getLocalBounds();

        for (size_t i = 0; i < macros.size(); ++i)
        {
            auto row = rowArea.removeFromTop (rowHeight);
            macroGrips[i]->setBounds (row.removeFromRight (16).withSizeKeepingCentre (16, 18));
            macros[i]->setJustification (RectSliderType::LeftJustifified);
            macros[i]->setBounds (row);
        }

        for (auto& modulator : modulators)
            modulator->button.setBounds (rowArea.removeFromTop (rowHeight).reduced (0, 2));

        auto& shown = *modulators[(size_t) selected];
        shown.type.setBounds (bounds.removeFromTop (22).removeFromLeft (110));
        bounds.removeFromTop (2);

        std::vector<RectSlider*> sliders;

        for (auto& slider : shown.sliders)
            if (slider->isVisible())
                sliders.push_back (slider.get());

        // two columns, the first taking the odd one out
        const auto firstColumn = ((int) sliders.size() + 1) / 2;
        const auto rowsHigh = bounds.getHeight() / juce::jmax (1, firstColumn);
        auto left = bounds.removeFromLeft (bounds.getWidth() / 2);

        placeColumn (left.withHeight (rowsHigh * firstColumn), sliders.data(), firstColumn, RectSliderType::LeftJustifified);
        placeColumn (bounds.withHeight (rowsHigh * ((int) sliders.size() - firstColumn)), sliders.data() + firstColumn,
                     (int) sliders.size() - firstColumn, RectSliderType::LeftJustifified);
    }

private:
    struct Modulator
    {
        Modulator (AudioPluginAudioProcessor& p, int m, ModulationStrip& strip)
            : button (p.getModMatrix(), ModSources::mod1 + m, true)
        {
            const char* const names[] { "RATE", "SHAPE", "ATTACK", "RELEASE", "GAIN", "RECTIFY" };
            static_assert (std::size (names) == ParamIDs::numModulatorParams);

            for (int k = 0; k < ParamIDs::numModulatorParams; ++k)
            {
                sliders[(size_t) k] = std::make_unique<RectSlider> (p, names[k], ParamIDs::modulatorParam (m, k));
                strip.addChildComponent (*sliders[(size_t) k]);
            }

            button.onClick = [&strip, m] { strip.select (m); };
            strip.list.addAndMakeVisible (button);

            // by index, since the menu has fewer items than the parameter has choices, see Module
            type.addItemList (ParamIDs::modulatorTypes.categories, 1);
            strip.addChildComponent (type);

            auto& parameter = *dynamic_cast<juce::AudioParameterChoice*> (p.treeState.getParameter (ParamIDs::modulatorType (m).getParamID()));
            type.setSelectedItemIndex (parameter.getIndex(), juce::dontSendNotification);

            // only once every modulator's been made, so the strip can lay them all out again
            typeAttachment = std::make_unique<juce::ParameterAttachment> (parameter, [this, &strip] (float index)
            {
                type.setSelectedItemIndex ((int) index, juce::dontSendNotification);
                button.repaint();
                strip.select (strip.selected);
            }, nullptr);

            type.onChange = [this] { typeAttachment->setValueAsCompleteGesture ((float) type.getSelectedItemIndex()); };
        }

        // which settings the type has
        bool uses (int param) const
        {
            switch (type.getSelectedItemIndex())
            {
                case ParamIDs::lfo:      return param == ParamIDs::modRate || param == ParamIDs::modShape;
                case ParamIDs::envelope: return param == ParamIDs::modAttack || param == ParamIDs::modRelease || param == ParamIDs::modGain;
                case ParamIDs::audio:    return param == ParamIDs::modGain || param == ParamIDs::modRectify;
                default:                 return false; // velocity and keytrack have nothing to set
            }
        }

        ModSourceButton button;
        juce::ComboBox type;
        std::array<std::unique_ptr<RectSlider>, ParamIDs::numModulatorParams> sliders;
        std::unique_ptr<juce::ParameterAttachment> typeAttachment;
    };

    void select (int modulator)
    {
        selected = modulator;

        for (int m = 0; m < (int) modulators.size(); ++m)
        {
            auto& each = *modulators[(size_t) m];
            each.button.setSelected (m == selected);
            each.type.setVisible (m == selected);

            for (int k = 0; k < ParamIDs::numModulatorParams; ++k)
                each.sliders[(size_t) k]->setVisible (m == selected && each.uses (k));
        }

        resized();
    }

    static constexpr int listWidth = 130, rowHeight = 24;

    juce::Viewport viewport;
    juce::Component list;

    std::array<std::unique_ptr<RectSlider>, 1 + ParamIDs::numGlobalMacros> macros;
    std::array<std::unique_ptr<ModSourceButton>, 1 + ParamIDs::numGlobalMacros> macroGrips;
    std::array<std::unique_ptr<Modulator>, ParamIDs::numModulators> modulators;
    int selected = 0;
};

// the bar across the plugin that opens and shuts the modulation row, its arrow pointing the way it'll go
class ModulationToggle : public juce::Button
{
public:
    ModulationToggle() : juce::Button ("modulation")
    {
        setClickingTogglesState (true);
        setTooltip ("Show or hide the macros and modulators");
    }

    void paintButton (juce::Graphics& g, bool hovered, bool) override
    {
        const auto box = getLocalBounds().toFloat().reduced ((float) Panel::boxInset, 2.0f);

        g.setColour (theme().box);
        g.fillRoundedRectangle (box, juce::jmin (Panel::boxCornerSize, box.getHeight() * 0.5f));

        const auto font = modFont (*this, 13.0f);
        const juce::String text ("MODULATION");
        constexpr auto arrowWidth = 8.0f, gap = 6.0f;

        const auto width = arrowWidth + gap + juce::GlyphArrangement::getStringWidth (font, text);
        auto content = box.withSizeKeepingCentre (width, box.getHeight());
        const auto arrow = content.removeFromLeft (arrowWidth).withSizeKeepingCentre (arrowWidth, 4.0f);
        content.removeFromLeft (gap);

        g.setColour (getToggleState() ? theme().textHeader : hovered ? theme().textHeaderHover : theme().textHeaderIdle);

        juce::Path chevron;
        const auto tip = getToggleState() ? arrow.getY() : arrow.getBottom();
        const auto base = getToggleState() ? arrow.getBottom() : arrow.getY();
        chevron.startNewSubPath (arrow.getX(), base);
        chevron.lineTo (arrow.getCentreX(), tip);
        chevron.lineTo (arrow.getRight(), base);
        g.strokePath (chevron, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setFont (font);
        g.drawText (text, content, juce::Justification::centredLeft, false);
    }
};

// -1 to 1, filled from the middle, with its value written over it
class ModBarSlider : public juce::Slider
{
public:
    explicit ModBarSlider (juce::String tooltip)
    {
        setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        setRange (-1.0, 1.0);
        setDoubleClickReturnValue (true, 0.0);
        setMouseDragSensitivity (200);
        setTooltip (tooltip);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (2.0f, 0.0f);
        const auto bar = bounds.removeFromBottom (4.0f);
        const auto proportion = (float) valueToProportionOfLength (getValue());

        g.setColour (theme().sliderTrack);
        g.fillRoundedRectangle (bar, 2.0f);

        g.setColour (theme().modulationHighlight);
        const auto middle = bar.getCentreX(), at = bar.getX() + proportion * bar.getWidth();
        g.fillRect (juce::Rectangle<float>::leftTopRightBottom (juce::jmin (middle, at), bar.getY(), juce::jmax (middle, at), bar.getBottom()));

        g.setColour (theme().textInfo);
        g.setFont (modFont (*this, 12.0f));
        g.drawText (juce::String (juce::roundToInt (getValue() * 100.0)) + "%", bounds, juce::Justification::centred, false);
    }
};

// the MOD page: every connection as a row, with a new one added from the bottom
class ModulationPage : public ModulationArea, private juce::ChangeListener
{
public:
    explicit ModulationPage (AudioPluginAudioProcessor& p) : processor (p), matrix (p.getModMatrix())
    {
        add.setTooltip ("Add a modulation, then pick its source and what it modulates. Or drag a source onto a knob");
        add.onClick = [this]
        {
            auto tree = matrix.getTree();

            if (tree.getNumChildren() < ModMatrix::maxConnections)
                tree.appendChild ({ ModMatrix::connectionId, { { ModMatrix::sourceId, ModSources::ids[ModSources::macro1] }, { ModMatrix::destId, "" },
                                                               { ModMatrix::amountId, 0.3f }, { ModMatrix::bipolarId, false }, { ModMatrix::skewId, 0.0f } } }, nullptr);
        };

        viewport.setViewedComponent (&list, false);
        viewport.setScrollBarsShown (true, false);
        addAndMakeVisible (viewport);
        addAndMakeVisible (add);

        matrix.addChangeListener (this);
        changeListenerCallback (nullptr);
    }

    ~ModulationPage() override { matrix.removeChangeListener (this); }

    void paint (juce::Graphics& g) override
    {
        ModulationArea::paint (g);

        g.setColour (theme().textSettingsDim);
        g.setFont (modFont (*this, 12.0f));

        auto header = headerArea();

        for (const auto& [title, width] : columns())
            g.drawText (title, header.removeFromLeft (width), juce::Justification::centredLeft, false);
    }

    void resized() override
    {
        auto bounds = content();
        bounds.removeFromTop (headerHeight);

        add.setBounds (bounds.removeFromBottom (24).removeFromLeft (80));
        bounds.removeFromBottom (4);
        viewport.setBounds (bounds);

        list.setBounds (0, 0, bounds.getWidth() - viewport.getScrollBarThickness(), (int) rows.size() * rowHeight);

        for (size_t i = 0; i < rows.size(); ++i)
            rows[i]->setBounds (0, (int) i * rowHeight, list.getWidth(), rowHeight);
    }

private:
    static constexpr int rowHeight = 28, headerHeight = 18;

    juce::Rectangle<int> content() const { return getLocalBounds().reduced (Panel::boxInset + 10); }
    juce::Rectangle<int> headerArea() const { return content().removeFromTop (headerHeight); }

    static std::vector<std::pair<juce::String, int>> columns()
    {
        return { { "SOURCE", 92 }, { "DESTINATION", 120 }, { "AMOUNT", 52 }, { "SKEW", 52 }, { "", 30 }, { "", 22 } };
    }

    struct Row : public juce::Component
    {
        Row (AudioPluginAudioProcessor& p, juce::ValueTree c) : connection (c), processor (p)
        {
            for (int s = 0; s < ModSources::count; ++s)
                source.addItem (p.getModMatrix().sourceName (s), s + 1);

            source.setTooltip ("What modulates. The modulators are named for the type each is set to under the distortion box");

            source.setSelectedId (ModSources::indexOf (connection[ModMatrix::sourceId]) + 1, juce::dontSendNotification);
            source.onChange = [this] { connection.setProperty (ModMatrix::sourceId, ModSources::ids[source.getSelectedId() - 1], nullptr); };

            auto* param = p.treeState.getParameter (connection[ModMatrix::destId].toString());
            dest.setButtonText (param != nullptr ? param->getName (40) : "CHOOSE...");
            dest.setTooltip (dest.getButtonText());
            dest.onClick = [this] { chooseDestination(); };

            amount.getValueObject().referTo (connection.getPropertyAsValue (ModMatrix::amountId, nullptr));
            skew.getValueObject().referTo (connection.getPropertyAsValue (ModMatrix::skewId, nullptr));

            bipolar.setClickingTogglesState (true);
            bipolar.getToggleStateValue().referTo (connection.getPropertyAsValue (ModMatrix::bipolarId, nullptr));
            bipolar.setTooltip ("Bipolar: move the parameter both ways from where it's set, rather than only up");
            bipolar.setColour (juce::TextButton::buttonOnColourId, theme().modulationHighlight);
            bipolar.setColour (juce::TextButton::textColourOnId, juce::Colours::black);

            remove.onClick = [this] { connection.getParent().removeChild (connection, nullptr); };

            for (auto* child : std::initializer_list<juce::Component*> { &source, &dest, &amount, &skew, &bipolar, &remove })
                addAndMakeVisible (child);
        }

        // as the modulators' types and the macros' names are now
        void refreshNames()
        {
            for (int s = 0; s < ModSources::count; ++s)
                source.changeItemText (s + 1, processor.getModMatrix().sourceName (s));

            source.setSelectedId (source.getSelectedId(), juce::dontSendNotification);
        }

        void resized() override
        {
            auto bounds = getLocalBounds().reduced (0, 3);
            juce::Component* const cells[] { &source, &dest, &amount, &skew, &bipolar, &remove };
            const auto widths = columns();

            for (size_t i = 0; i < std::size (cells); ++i)
                cells[i]->setBounds (bounds.removeFromLeft (widths[i].second).withTrimmedRight (4));
        }

        // every parameter a source can go to, grouped by the box it's in
        void chooseDestination()
        {
            std::map<juce::String, juce::PopupMenu> groups;
            for (auto* parameter : processor.getParameters())
            {
                auto* param = dynamic_cast<MacroParam*> (parameter);

                if (param == nullptr || ! processor.getModMatrix().canModulate (param->getParameterID()))
                    continue;

                const auto id = param->getParameterID();
                const auto own = param->getDescriptor().displayName;
                const auto prefix = id.dropLastCharacters (param->getDescriptor().getParamID().length() + 1);
                auto group = juce::String ("GLOBAL");

                for (const auto& module : pluginModules)
                    for (int sub = 0; sub < module.slotCount; ++sub)
                        if (SlotId { module.id, sub }.prefix() == prefix)
                            group = SlotId { module.id, sub }.displayNamePrefix();

                groups[group].addItem (own, [safe = juce::Component::SafePointer<Row> (this), id]
                {
                    if (safe != nullptr)
                        safe->connection.setProperty (ModMatrix::destId, id, nullptr);
                });
            }

            juce::PopupMenu menu;
            menu.setLookAndFeel (&getLookAndFeel());

            for (auto& [group, items] : groups)
                menu.addSubMenu (group, items);

            menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&dest));
        }

        struct BinButton : juce::Button
        {
            BinButton() : juce::Button ("remove") { setTooltip ("Remove this modulation"); }

            void paintButton (juce::Graphics& g, bool hovered, bool) override
            {
                static const auto bin = juce::Drawable::parseSVGPath ("M4 7h16M10 11v6M14 11v6M5 7l1 12a2 2 0 0 0 2 2h8a2 2 0 0 0 2-2l1-12M9 7V4a1 1 0 0 1 1-1h4a1 1 0 0 1 1 1v3");
                drawGlowingGlyph (g, bin, getLocalBounds().toFloat(), hovered ? theme().multibandRemoveHot : theme().multibandRemove);
            }
        };

        juce::ValueTree connection;
        AudioPluginAudioProcessor& processor;

        juce::ComboBox source;
        juce::TextButton dest, bipolar { "BI" };
        ModBarSlider amount { "How far the source moves the parameter, across its whole range at 100%. Keytracking a frequency, an octave per octave at 100%" },
                     skew { "Bends the source towards its ends or its middle" };
        BinButton remove;
    };

    // new rows only when connections came or went, so dragging an amount doesn't rebuild the row it's in
    void changeListenerCallback (juce::ChangeBroadcaster*) override
    {
        const auto tree = matrix.getTree();
        auto same = (int) rows.size() == tree.getNumChildren();

        for (size_t i = 0; same && i < rows.size(); ++i)
            same = rows[i]->connection == tree.getChild ((int) i);

        if (same)
        {
            for (auto& row : rows)
                row->refreshNames();

            return;
        }

        rows.clear();

        for (const auto& connection : tree)
        {
            rows.push_back (std::make_unique<Row> (processor, connection));
            list.addAndMakeVisible (*rows.back());
        }

        resized();
    }

    AudioPluginAudioProcessor& processor;
    ModMatrix& matrix;

    juce::Viewport viewport;
    juce::Component list;
    std::vector<std::unique_ptr<Row>> rows;
    juce::TextButton add { "+ ADD" };
};
