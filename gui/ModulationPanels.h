#pragma once

#include "RectSlider.h"
#include "BoxToggle.h"
#include "Modules/Panel.h"
#include "Modules/LightButton.h"
#include "LookAndFeel/HamburgerLAF.h"

static_assert (ModSources::count == std::tuple_size_v<decltype (Theme::modSources)>, "a theme colour for every mod source");

inline juce::Font modFont(juce::Component& component, float height)
{
    if (auto* laf = dynamic_cast<HamburgerLAF*>(&component.getLookAndFeel()))
        return laf->getQuicksandFont().withHeight(height);

    return juce::Font(juce::FontOptions(height));
}

// the modulation boxes' menus, in the category text like every box's type menu, whatever theme it's switched to
struct ModComboBox : juce::ComboBox
{
    ModComboBox() { lookAndFeelChanged(); }

    void lookAndFeelChanged() override
    {
        setColour(textColourId, theme().categoryText);
        setColour(arrowColourId, theme().categoryText);
        juce::ComboBox::lookAndFeelChanged();
    }
};

// a box like the others, without a power button or type of its own
class ModulationArea : public juce::Component
{
public:
    void paint(juce::Graphics& g) override
    {
        const auto box = getLocalBounds().reduced(Panel::boxInset).toFloat();

        g.setColour(theme().box);
        g.fillRoundedRectangle(box, Panel::boxCornerSize);

        if (theme().boxBorders)
        {
            g.setColour(theme().boxBorder);
            g.drawRoundedRectangle(box.reduced(0.5f), Panel::boxCornerSize, 1.0f);
        }
    }
};

/*  A source to drag onto any knob or slider, routing it there: a button in the source's colour with a short name.
    Clicked it's selected, double clicked it's armed for mod edit mode, and filled in while it is. */
class ModSourceButton : public juce::Component, public juce::TooltipClient
{
public:
    ModSourceButton(const ModMatrix& m, int sourceIndex, juce::String shortName) : matrix(m), source(sourceIndex), label(shortName)
    {
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }

    std::function<void()> onClick, onDoubleClick;

    void setSelected(bool shouldBeSelected)
    {
        selected = shouldBeSelected;
        repaint();
    }

    // the full name, which a macro's rename or a modulator's type changes
    juce::String getTooltip() override
    {
        return matrix.sourceName(source) + ". Click to edit, double click for mod edit mode, or drag onto a knob";
    }

    void paint(juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat().reduced(1.5f);
        const auto armed = matrix.getArmedSource() == source;

        // a modulator is only in its colour while it's picked or armed, the rest greyed out, so it's clear whose modulation the controls show
        const auto greyed = source >= ModSources::mod1 && ! selected && ! armed;
        const auto colour = greyed ? theme().textDim : theme().modSources[(size_t) source];

        g.setColour(armed ? colour : colour.withAlpha(selected ? 0.35f : isMouseOver() ? 0.25f : 0.15f));
        g.fillRoundedRectangle(bounds, 5.0f);

        g.setColour(colour);
        g.drawRoundedRectangle(bounds, 5.0f, selected ? 2.0f : 1.2f);

        g.setColour(armed ? theme().box : selected ? theme().textHeader : colour);
        g.setFont(modFont(*this, 13.0f));
        g.drawFittedText(label, bounds.toNearestInt(), juce::Justification::centred, 1, 0.8f);
    }

    void mouseEnter(const juce::MouseEvent&) override { repaint(); }
    void mouseExit(const juce::MouseEvent&) override { repaint(); }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (dragging || e.getDistanceFromDragStart() < 4)
            return;

        if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this))
        {
            dragging = true;
            container->startDragging(GenericKnob::modDragPrefix + juce::String(source), this);
        }
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        if (! dragging && onClick != nullptr)
            onClick();

        dragging = false;
    }

    void mouseDoubleClick(const juce::MouseEvent&) override
    {
        if (onDoubleClick != nullptr)
            onDoubleClick();
    }

private:
    const ModMatrix& matrix;
    const int source;
    const juce::String label;
    bool selected = false, dragging = false;
};

/*  The modulation box, between the pre fx and the noise: two columns of buttons down the left, the drive and the macros
    then the modulators. Beside them whatever's picked: a modulator shows its type, power and settings, and the drive or
    a macro shows the drive as a knob and the two macros as sliders. */
class ModulationBox : public ModulationArea, private juce::ChangeListener
{
public:
    explicit ModulationBox(AudioPluginAudioProcessor& p) : processor(p), matrix(p.getModMatrix()), scopeContext(p.getScopeContext())
    {
        const char* const labels[] { "D", "M1", "M2", "A", "B", "C" };
        static_assert (std::size(labels) == ModSources::numInBox);

        for (int s = 0; s < ModSources::numInBox; ++s)
        {
            auto& button = *(buttons[(size_t) s] = std::make_unique<ModSourceButton>(matrix, s, labels[s]));
            button.onClick = [this, s] { select(s < ModSources::mod1 ? ModSources::drive : s); };

            // again on the armed one switches mod edit mode back off
            button.onDoubleClick = [this, s] { matrix.setArmedSource(matrix.getArmedSource() == s ? -1 : s); };
            addAndMakeVisible(button);
        }

        for (int i = 0; i < ParamIDs::numGlobalMacros; ++i)
        {
            macros[(size_t) i] = std::make_unique<RectSlider>(p, "MACRO " + juce::String(i + 1), *ParamIDs::globalMacros[i]);
            macros[(size_t) i]->setJustification(RectSliderType::LeftJustifified);
            addChildComponent(*macros[(size_t) i]);
        }

        addChildComponent(drive);

        for (int m = 0; m < ParamIDs::numModulators; ++m)
            modulators[(size_t) m] = std::make_unique<Modulator>(p, m, *this);

        matrix.addChangeListener(this);
        select(ModSources::drive);
    }

    ~ModulationBox() override { matrix.removeChangeListener(this); }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(Panel::boxInset + 8, Panel::boxInset + 6);

        // the drive and the macros down the first column, the modulators down the second
        auto grid = bounds.removeFromLeft(buttonWidth * 2 + gap);
        const auto rowHeight = grid.getHeight() / 3;

        for (int s = 0; s < ModSources::numInBox; ++s)
        {
            const auto isModulator = s >= ModSources::mod1;
            buttons[(size_t) s]->setBounds(grid.getX() + (isModulator ? buttonWidth + gap : 0), grid.getY() + (isModulator ? s - ModSources::mod1 : s) * rowHeight,
                                           buttonWidth, rowHeight - 2);
        }

        bounds.removeFromLeft(gap * 2);

        // the drive as big as the height allows, and the macros one above the other beside it
        if (selected <= ModSources::macro2)
        {
            const auto driveWidth = bounds.getHeight();
            drive.setBounds(bounds.removeFromLeft(driveWidth));
            bounds.removeFromLeft(gap * 2);

            const auto cellHeight = bounds.getHeight() / 2;

            for (int i = 0; i < (int) macros.size(); ++i)
            {
                // only as tall as what it draws, which sits at its bottom, so it's centred in its cell
                const auto cell = bounds.withY(bounds.getY() + i * cellHeight).withHeight(cellHeight);
                macros[(size_t) i]->setBounds(cell.withSizeKeepingCentre(cell.getWidth(), juce::roundToInt(macros[(size_t) i]->visibleHeight())));
            }

            return;
        }

        // the power, the type and an LFO's shape along the top, and the settings that type has as knobs under them
        auto& shown = *modulators[(size_t) (selected - ModSources::mod1)];
        auto top = bounds.removeFromTop(topRowHeight);

        shown.power.setBounds(top.removeFromLeft(topRowHeight).reduced(3));
        top.removeFromLeft(gap);
        shown.type.setBounds(top.removeFromLeft(typeWidth));
        top.removeFromLeft(gap);
        shown.shape.setBounds(top.removeFromLeft(typeWidth));
        shown.randomType.setBounds(shown.shape.getBounds());
        top.removeFromLeft(gap * 2);
        shown.sawDown.setBounds(top.removeFromLeft(typeWidth));

        // whichever of the three rates it's using first, so switching between them leaves the rate where it was
        using namespace ParamIDs;
        constexpr int order[] { modRate, modRateWide, modSyncRate, modShape, modAttack, modRelease, modGain, modRectify, modStereo, modRandomType, modSmooth };
        static_assert (std::size(order) == numModulatorParams);

        std::vector<juce::Component*> knobs;

        for (const auto k : order)
            if (shown.knobs[(size_t) k]->isVisible())
                knobs.push_back(shown.knobs[(size_t) k].get());

        if (knobs.empty())
            return;

        const auto width = bounds.getWidth() / juce::jmax(3, (int) knobs.size());

        for (auto* knob : knobs)
            knob->setBounds(bounds.removeFromLeft(width));
    }

private:
    struct Modulator
    {
        Modulator(AudioPluginAudioProcessor& p, int m, ModulationBox& box)
            : powerAttachment(p.treeState, ParamIDs::modulatorToggle(m, ParamIDs::modOn).getParamID(), power),
              sawDownAttachment(p.treeState, ParamIDs::modulatorToggle(m, ParamIDs::modSawDown).getParamID(), sawDown)
        {
            const char* const names[] { "RATE", "SHAPE", "ATTACK", "RELEASE", "GAIN", "RECTIFY", "RATE", "STEREO", "RATE", "TYPE", "SMOOTH" };
            static_assert (std::size(names) == ParamIDs::numModulatorParams);

            for (int k = 0; k < ParamIDs::numModulatorParams; ++k)
            {
                knobs[(size_t) k] = std::make_unique<ParamKnob>(p, names[k], ParamIDs::modulatorParam(m, k), ScopeContextType::MODULATOR);
                box.addChildComponent(*knobs[(size_t) k]);
            }

            power.setTooltip("Switches this modulator off, keeping where it's routed");
            box.addChildComponent(power);

            // every rate knob's right click menu ticks the tempo sync and the bigger range, which swap which of them shows.
            // synced, the range is greyed out, the tempo deciding how fast it goes
            for (auto which : { ParamIDs::modWideRange, ParamIDs::modSync })
            {
                toggles[(size_t) which] = dynamic_cast<juce::AudioParameterBool*>(p.treeState.getParameter(ParamIDs::modulatorToggle(m, which).getParamID()));
                toggleAttachments[(size_t) which] = std::make_unique<juce::ParameterAttachment>(*toggles[(size_t) which], [&box](float) { box.select(box.selected); }, nullptr);
            }

            for (auto which : { ParamIDs::modRate, ParamIDs::modRateWide, ParamIDs::modSyncRate })
                knobs[(size_t) which]->addToMenu = [this](juce::PopupMenu& menu)
                {
                    menu.addItem("BPM sync", true, isOn(ParamIDs::modSync), [this] { flip(ParamIDs::modSync); });
                    menu.addItem("Bigger freq range", ! isOn(ParamIDs::modSync), isOn(ParamIDs::modWideRange), [this] { flip(ParamIDs::modWideRange); });
                };

            sawDown.setTooltip(ParamIDs::modulatorToggle(m, ParamIDs::modSawDown).paramTooltip);
            box.addChildComponent(sawDown);

            // the shape parameter morphs across these four, the menu picks each one exactly
            shape.addItemList({ "SINE", "TRIANGLE", "SAW", "SQUARE" }, 1);
            shape.setTooltip("The LFO's wave");
            box.addChildComponent(shape);

            // only the saw has a direction. set here rather than through the box, which is still making the modulators on the initial update
            auto& shapeParameter = *p.treeState.getParameter(ParamIDs::modulatorParam(m, ParamIDs::modShape).getParamID());
            shapeAttachment = std::make_unique<juce::ParameterAttachment>(shapeParameter, [this](float value)
            {
                shape.setSelectedItemIndex(juce::roundToInt(value * (float) (shape.getNumItems() - 1)), juce::dontSendNotification);
                sawDown.setVisible(shape.isVisible() && shape.getText() == "SAW");
            }, nullptr);

            shapeAttachment->sendInitialUpdate();
            shape.onChange = [this]
            {
                shapeAttachment->setValueAsCompleteGesture((float) shape.getSelectedItemIndex() / (float) (shape.getNumItems() - 1));
            };

            // the random's algorithm, a stepped parameter by index
            randomType.addItemList(ParamIDs::randomTypes, 1);
            randomType.setTooltip(ParamIDs::modulatorParam(m, ParamIDs::modRandomType).paramTooltip);
            box.addChildComponent(randomType);

            randomTypeAttachment = std::make_unique<juce::ParameterAttachment>(*p.treeState.getParameter(ParamIDs::modulatorParam(m, ParamIDs::modRandomType).getParamID()),
                                                                                [this](float index) { randomType.setSelectedItemIndex((int) index, juce::dontSendNotification); }, nullptr);
            randomTypeAttachment->sendInitialUpdate();
            randomType.onChange = [this] { randomTypeAttachment->setValueAsCompleteGesture((float) randomType.getSelectedItemIndex()); };

            // by index, since the menu has fewer items than the parameter has choices, see Module
            type.addItemList(ParamIDs::modulatorTypes.categories, 1);
            box.addChildComponent(type);

            auto& parameter = *dynamic_cast<juce::AudioParameterChoice*>(p.treeState.getParameter(ParamIDs::modulatorType(m).getParamID()));
            type.setSelectedItemIndex(parameter.getIndex(), juce::dontSendNotification);

            typeAttachment = std::make_unique<juce::ParameterAttachment>(parameter, [this, &box](float index)
            {
                type.setSelectedItemIndex((int) index, juce::dontSendNotification);
                box.select(box.selected);
            }, nullptr);

            type.onChange = [this] { typeAttachment->setValueAsCompleteGesture((float) type.getSelectedItemIndex()); };
        }

        bool isOn(ParamIDs::ModulatorToggle which) const { return toggles[(size_t) which]->get(); }
        void flip(ParamIDs::ModulatorToggle which) { toggleAttachments[(size_t) which]->setValueAsCompleteGesture(isOn(which) ? 0.0f : 1.0f); }

        // which settings the type has
        bool uses(int param) const
        {
            const auto rate = isOn(ParamIDs::modSync) ? ParamIDs::modSyncRate : isOn(ParamIDs::modWideRange) ? ParamIDs::modRateWide : ParamIDs::modRate;

            switch (type.getSelectedItemIndex())
            {
                // the rate it's set to use and its stereo spread, and its shape is the menu beside its type
                case ParamIDs::lfo:      return param == rate || param == ParamIDs::modStereo;
                case ParamIDs::random:   return param == rate || param == ParamIDs::modSmooth;
                case ParamIDs::envelope: return param == ParamIDs::modAttack || param == ParamIDs::modRelease || param == ParamIDs::modGain;
                case ParamIDs::audio:    return param == ParamIDs::modGain || param == ParamIDs::modRectify;
                default:                 return false;
            }
        }

        ModComboBox type, shape, randomType;
        std::unique_ptr<juce::ParameterAttachment> shapeAttachment, randomTypeAttachment;
        std::array<juce::AudioParameterBool*, ParamIDs::numModulatorToggles> toggles {}; // the wide range and the sync
        std::array<std::unique_ptr<juce::ParameterAttachment>, ParamIDs::numModulatorToggles> toggleAttachments;
        LightButton power { powerGlyph(), &Theme::powerOn, &Theme::powerOff };
        BoxToggle sawDown { "DOWN" };
        juce::AudioProcessorValueTreeState::ButtonAttachment powerAttachment, sawDownAttachment;
        std::array<std::unique_ptr<ParamKnob>, ParamIDs::numModulatorParams> knobs;
        std::unique_ptr<juce::ParameterAttachment> typeAttachment;
    };

    void select(int source)
    {
        selected = source;
        const auto showsMacros = selected <= ModSources::macro2;

        for (int s = 0; s < ModSources::numInBox; ++s)
            buttons[(size_t) s]->setSelected(s >= ModSources::mod1 && s == selected);

        drive.setVisible(showsMacros);

        for (auto& macro : macros)
            macro->setVisible(showsMacros);

        for (int m = 0; m < (int) modulators.size(); ++m)
        {
            auto& each = *modulators[(size_t) m];
            const auto shown = selected == ModSources::mod1 + m;

            each.type.setVisible(shown);
            each.power.setVisible(shown);
            each.shape.setVisible(shown && each.type.getSelectedItemIndex() == ParamIDs::lfo);
            each.randomType.setVisible(shown && each.type.getSelectedItemIndex() == ParamIDs::random);
            each.sawDown.setVisible(each.shape.isVisible() && each.shape.getText() == "SAW");

            for (int k = 0; k < ParamIDs::numModulatorParams; ++k)
                each.knobs[(size_t) k]->setVisible(shown && each.uses(k));
        }

        // the controls only draw what the picked modulator does
        matrix.setSelectedSource(showsMacros ? -1 : selected);

        // picking a modulator shows its output in the scope for a moment, like touching a knob shows its view
        if (! showsMacros)
        {
            scopeContext.setModulatorSource(selected);
            scopeContext.setType(ScopeContextType::MODULATOR);
            scopeContext.startDecaying();
        }

        resized();
        repaint();
    }

    // arming and disarming mod edit mode, and the names a rename or a type change gives the sources
    void changeListenerCallback(juce::ChangeBroadcaster*) override
    {
        for (auto& button : buttons)
            button->repaint();
    }

    static constexpr int buttonWidth = 34, gap = 4, topRowHeight = 22, typeWidth = 110;

    AudioPluginAudioProcessor& processor;
    ModMatrix& matrix;
    ScopeContext& scopeContext;
    std::array<std::unique_ptr<ModSourceButton>, ModSources::numInBox> buttons;
    ParamKnob drive { processor, "DRIVE", ParamIDs::globalDrive };
    std::array<std::unique_ptr<RectSlider>, ParamIDs::numGlobalMacros> macros;
    std::array<std::unique_ptr<Modulator>, ParamIDs::numModulators> modulators;
    int selected = ModSources::drive;
};

// -1 to 1 filled from the middle, or 0 to 1 from the left, with its value written over it
class ModBarSlider : public juce::Slider
{
public:
    explicit ModBarSlider(juce::String tooltip, bool fromMiddle = true)
    {
        setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
        setRange(fromMiddle ? -1.0 : 0.0, 1.0);
        setDoubleClickReturnValue(true, 0.0);
        setMouseDragSensitivity(200);
        setTooltip(tooltip);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f, 0.0f);
        const auto bar = bounds.removeFromBottom(4.0f);
        const auto proportion = (float) valueToProportionOfLength(getValue());

        g.setColour(theme().sliderTrack);
        g.fillRoundedRectangle(bar, 2.0f);

        g.setColour(theme().modulationHighlight);
        const auto middle = getMinimum() < 0.0 ? bar.getCentreX() : bar.getX(), at = bar.getX() + proportion * bar.getWidth();
        g.fillRect(juce::Rectangle<float>::leftTopRightBottom(juce::jmin(middle, at), bar.getY(), juce::jmax(middle, at), bar.getBottom()));

        g.setColour(theme().knobText);
        g.setFont(modFont(*this, 12.0f));
        g.drawText(juce::String(juce::roundToInt(getValue() * 100.0)) + "%", bounds, juce::Justification::centred, false);
    }
};

// the MOD page: every connection as a row, with a new one added from the bottom
class ModulationPage : public ModulationArea, private juce::ChangeListener, private juce::Timer
{
public:
    explicit ModulationPage(AudioPluginAudioProcessor& p) : processor(p), matrix(p.getModMatrix())
    {
        add.setTooltip("Add a modulation, or drag a source onto a knob");
        add.onClick = [this]
        {
            auto tree = matrix.getTree();

            if (tree.getNumChildren() < ModMatrix::maxConnections)
                tree.appendChild({ ModMatrix::connectionId, { { ModMatrix::sourceId, ModSources::ids[ModSources::macro1] }, { ModMatrix::destId, "" },
                                                               { ModMatrix::amountId, 0.3f } } }, nullptr);
        };

        viewport.setViewedComponent(&list, false);
        viewport.setScrollBarsShown(true, false);
        addAndMakeVisible(viewport);
        addAndMakeVisible(add);

        matrix.addChangeListener(this);
        changeListenerCallback(nullptr);

        // a slot switched to another type leaves what's routed to its old one doing nothing, which its row shows
        startTimerHz(5);
    }

    ~ModulationPage() override { matrix.removeChangeListener(this); }

    void paint(juce::Graphics& g) override
    {
        ModulationArea::paint(g);

        g.setColour(theme().textDim.interpolatedWith(theme().labelText, 0.5f));
        g.setFont(modFont(*this, 14.0f));

        auto header = headerArea();

        for (const auto& [title, width] : columns(header.getWidth() - viewport.getScrollBarThickness()))
            g.drawText(title, header.removeFromLeft(width), juce::Justification::centredLeft, false);
    }

    void resized() override
    {
        auto bounds = content();
        bounds.removeFromTop(headerHeight);

        add.setBounds(bounds.removeFromBottom(24).removeFromLeft(80));
        bounds.removeFromBottom(4);
        viewport.setBounds(bounds);

        list.setBounds(0, 0, bounds.getWidth() - viewport.getScrollBarThickness(), (int) rows.size() * rowHeight);

        for (size_t i = 0; i < rows.size(); ++i)
            rows[i]->setBounds(0, (int) i * rowHeight, list.getWidth(), rowHeight);
    }

private:
    static constexpr int rowHeight = 32, headerHeight = 20;

    juce::Rectangle<int> content() const { return getLocalBounds().reduced(Panel::boxInset + 10); }
    juce::Rectangle<int> headerArea() const { return content().removeFromTop(headerHeight); }

    // the buttons keep their size and the rest share the width, the destination most since its names run longest
    static std::vector<std::pair<juce::String, int>> columns(int width)
    {
        constexpr int toggleWidth = 62, removeWidth = 26;
        const auto shared = width - toggleWidth * 2 - removeWidth;

        return { { "SOURCE", shared * 20 / 100 }, { "DESTINATION", shared * 38 / 100 }, { "AMOUNT", shared * 16 / 100 },
                 { "SKEW", shared * 14 / 100 }, { "SMOOTH", shared - shared * 88 / 100 }, { "BIPOLAR", toggleWidth },
                 { "INVERT", toggleWidth }, { "", removeWidth } };
    }

    struct Row : public juce::Component
    {
        Row(AudioPluginAudioProcessor& p, juce::ValueTree c) : connection(c), processor(p)
        {
            for (int s = 0; s < ModSources::count; ++s)
                source.addItem(p.getModMatrix().sourceName(s), s + 1);

            source.setTooltip("What modulates. The modulators are named by their type");

            source.setSelectedId(ModSources::indexOf(connection[ModMatrix::sourceId]) + 1, juce::dontSendNotification);
            source.onChange = [this]
            {
                connection.setProperty(ModMatrix::sourceId, ModSources::ids[source.getSelectedId() - 1], nullptr);
                showInScope();
            };

            // every parameter a source can go to, each in a submenu for the box it's in
            for (auto* parameter : p.getParameters())
            {
                auto* param = dynamic_cast<MacroParam*>(parameter);

                if (param == nullptr || ! p.getModMatrix().canModulate(param->getParameterID()))
                    continue;

                const auto id = param->getParameterID();
                const auto prefix = id.dropLastCharacters(param->getDescriptor().getParamID().length() + 1);
                auto group = juce::String("GLOBAL");

                for (const auto& module : pluginModules)
                    for (int sub = 0; sub < module.slotCount; ++sub)
                        if (SlotId { module.id, sub }.prefix() == prefix)
                            group = SlotId { module.id, sub }.displayNamePrefix();

                destinations.add(id);
                destinationGroups.add(group);
                destinationNames.add(parameter->getName(40));
            }

            // only the boxes' types as they're set now, and whatever this row already goes to
            dest.beforePopup = [this] { fillDestinations(); };
            fillDestinations();

            dest.setTextWhenNothingSelected("CHOOSE...");
            dest.setTooltip("What the source modulates, from the types each box is set to");
            dest.onChange = [this]
            {
                if (const auto id = dest.getSelectedId(); id > 0)
                    connection.setProperty(ModMatrix::destId, destinations[id - 1], nullptr);

                showInScope();
            };

            amount.getValueObject().referTo(connection.getPropertyAsValue(ModMatrix::amountId, nullptr));
            skew.getValueObject().referTo(connection.getPropertyAsValue(ModMatrix::skewId, nullptr));
            smooth.getValueObject().referTo(connection.getPropertyAsValue(ModMatrix::smoothId, nullptr));
            bipolar.getToggleStateValue().referTo(connection.getPropertyAsValue(ModMatrix::bipolarId, nullptr));
            invert.getToggleStateValue().referTo(connection.getPropertyAsValue(ModMatrix::invertId, nullptr));

            bipolar.setTooltip("Move the parameter both ways from where it's set, rather than only up");
            invert.setTooltip("Flips the source: bipolar goes the other way, unipolar falls from amount");

            for (auto* slider : { &amount, &skew, &smooth })
                slider->onValueChange = [this] { showInScope(); };

            for (auto* toggle : { &bipolar, &invert })
                toggle->onClick = [this] { showInScope(); };

            remove.onClick = [this] { connection.getParent().removeChild(connection, nullptr); };

            for (auto* child : std::initializer_list<juce::Component*> { &source, &dest, &amount, &skew, &smooth, &bipolar, &invert, &remove })
                addAndMakeVisible(child);
        }

        // what this connection does to its source, in the scope for a moment
        void showInScope()
        {
            auto& scopeContext = processor.getScopeContext();
            scopeContext.setModulatorSource(ModSources::indexOf(connection[ModMatrix::sourceId]), connection.getParent().indexOf(connection));
            scopeContext.setType(ScopeContextType::MODULATOR);
            scopeContext.startDecaying();
        }

        void fillDestinations()
        {
            const auto current = connection[ModMatrix::destId].toString();
            std::map<juce::String, juce::PopupMenu> groups;

            for (int i = 0; i < destinations.size(); ++i)
                if (destinations[i] == current || processor.getModMatrix().isUsed(destinations[i]))
                    groups[destinationGroups[i]].addItem(i + 1, destinationNames[i]);

            dest.clear(juce::dontSendNotification);

            for (auto& [group, items] : groups)
                dest.getRootMenu()->addSubMenu(group, items);

            dest.setSelectedId(destinations.indexOf(current) + 1, juce::dontSendNotification);
        }

        // dimmed while its parameter's slot is set to a type that doesn't have it
        void updateUsed()
        {
            const auto current = connection[ModMatrix::destId].toString();
            setAlpha(current.isEmpty() || processor.getModMatrix().isUsed(current) ? 1.0f : 0.4f);
        }

        // as the modulators' types and the macros' names are now
        void refreshNames()
        {
            for (int s = 0; s < ModSources::count; ++s)
                source.changeItemText(s + 1, processor.getModMatrix().sourceName(s));

            source.setSelectedId(source.getSelectedId(), juce::dontSendNotification);
        }

        void resized() override
        {
            auto bounds = getLocalBounds().reduced(0, 3);
            juce::Component* const cells[] { &source, &dest, &amount, &skew, &smooth, &bipolar, &invert, &remove };
            const auto widths = columns(getWidth());

            for (size_t i = 0; i < std::size(cells); ++i)
                cells[i]->setBounds(bounds.removeFromLeft(widths[i].second).withTrimmedRight(4));
        }

        struct BinButton : juce::Button
        {
            BinButton() : juce::Button("remove") { setTooltip("Remove this modulation"); }

            void paintButton(juce::Graphics& g, bool hovered, bool) override
            {
                static const auto bin = juce::Drawable::parseSVGPath("M4 7h16M10 11v6M14 11v6M5 7l1 12a2 2 0 0 0 2 2h8a2 2 0 0 0 2-2l1-12M9 7V4a1 1 0 0 1 1-1h4a1 1 0 0 1 1 1v3");
                drawGlowingGlyph(g, bin, getLocalBounds().toFloat(), hovered ? theme().multibandRemoveHot : theme().multibandRemove);
            }
        };

        juce::ValueTree connection;
        AudioPluginAudioProcessor& processor;

        // fills its menu right as it opens, so it's always the types as they're set
        struct DestinationBox : ModComboBox
        {
            std::function<void()> beforePopup;

            void showPopup() override
            {
                beforePopup();
                juce::ComboBox::showPopup();
            }
        };

        ModComboBox source;
        DestinationBox dest;
        juce::StringArray destinations, destinationGroups, destinationNames; // the parameter ids, by the dest menu's item ids less one
        BoxToggle bipolar { "" }, invert { "" };
        ModBarSlider amount { "How far the source moves the parameter" },
                     skew { "Bends the source towards its ends or its middle" },
                     smooth { "Smooths the source out, taking up to a second to follow it. Off at 0%", false };
        BinButton remove;
    };

    void timerCallback() override
    {
        for (auto& row : rows)
            row->updateUsed();
    }

    // new rows only when connections came or went, so dragging an amount doesn't rebuild the row it's in
    void changeListenerCallback(juce::ChangeBroadcaster*) override
    {
        const auto tree = matrix.getTree();
        auto same = (int) rows.size() == tree.getNumChildren();

        for (size_t i = 0; same && i < rows.size(); ++i)
            same = rows[i]->connection == tree.getChild((int) i);

        if (same)
        {
            for (auto& row : rows)
                row->refreshNames();

            return;
        }

        rows.clear();

        for (const auto& connection : tree)
        {
            rows.push_back(std::make_unique<Row>(processor, connection));
            list.addAndMakeVisible(*rows.back());
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
