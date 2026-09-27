#pragma once

#include "../Panel.h"

// one of the start page's amounts. the knob comes with the power button of the box it stands in for, and that box's
// type colours, listed in the order of its type menu
class AmountKnob : public juce::Component
{
public:
    AmountKnob (AudioPluginAudioProcessor& p, const juce::String& title, const ParamIDs::ParameterInfo& amount, ScopeContextType scopeType,
                const juce::String& enabledId, const juce::String& typeId, std::vector<AccentColours Theme::*> typeColours)
        : knob (p, title, amount, scopeType),
          powerAttachment (p.treeState, enabledId, power),
          type (*dynamic_cast<juce::AudioParameterChoice*> (p.treeState.getParameter (typeId))),
          typeAttachment (type, [this] (float) { lookAndFeelChanged(); }),
          colours (std::move (typeColours))
    {
        power.setTooltip ("Enable / disable audio processing");

        // dimmed while off, like a box whose power button is off
        power.onStateChange = [this] { knob.setAlpha (power.getToggleState() ? 1.0f : 0.6f); };
        power.onStateChange();

        addAndMakeVisible (knob);
        addAndMakeVisible (power);
    }

    void lookAndFeelChanged() override
    {
        knob.setAccent (colours[(size_t) juce::jlimit (0, (int) colours.size() - 1, type.getIndex())]);
    }

    void resized() override
    {
        power.setBounds (0, 0, powerSize, powerSize);

        const auto size = juce::jmin (getWidth(), getHeight());
        knob.setBounds (getLocalBounds().withSizeKeepingCentre (size, size));
    }

private:
    static constexpr int powerSize = 15;

    ParamKnob knob;
    LightButton power { powerGlyph(), &Theme::powerOn, &Theme::powerOff };
    juce::AudioProcessorValueTreeState::ButtonAttachment powerAttachment;

    juce::AudioParameterChoice& type;
    juce::ParameterAttachment typeAttachment;
    std::vector<AccentColours Theme::*> colours;
};

// the start page's compressor, emphasis, noise and pre fx amounts, in a row
class AmountsPanel : public Panel
{
public:
    explicit AmountsPanel (AudioPluginAudioProcessor& p)
        : Panel (p, "AMOUNTS"),
          compressor (p, "COMP", ParamIDs::compAmount, ScopeContextType::COMPRESSION,
                      SlotId { ModuleId::dynamics, 0 }.enabled().getParamID(), SlotId { ModuleId::dynamics, 0 }.type().getParamID(),
                      { &Theme::stereoComp, &Theme::multibandComp, &Theme::midSideComp, &Theme::typeAComp, &Theme::transient, &Theme::multibandTransient, &Theme::optoComp, &Theme::multibandOptoComp }),
          eq (p, "EQ", ParamIDs::eqStrength, ScopeContextType::SPECTRUM_EMPHASIS,
              ParamIDs::emphasisOn.getParamID(), ParamIDs::emphasisType.getParamID(),
              { &Theme::emphasis, &Theme::tilt }),
          noise (p, "NOISE", ParamIDs::noiseAmount, ScopeContextType::NOISE,
                 SlotId { ModuleId::module1, 0 }.enabled().getParamID(), SlotId { ModuleId::module1, 0 }.type().getParamID(),
                 { &Theme::sizzle, &Theme::erosion, &Theme::bitReduction, &Theme::gate, &Theme::fizz }),
          preFx (p, "PRE FX", ParamIDs::preFxAmount, ScopeContextType::LR_SCOPE,
                 SlotId { ModuleId::module2, 0 }.enabled().getParamID(), SlotId { ModuleId::module2, 0 }.type().getParamID(),
                 { &Theme::allpass, &Theme::grunge, &Theme::subGen, &Theme::hilbertStack })
    {
        for (auto* amount : { &compressor, &eq, &noise, &preFx })
            addAndMakeVisible (amount);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        const auto width = bounds.getWidth() / 4;

        for (auto* amount : { &compressor, &eq, &noise, &preFx })
            amount->setBounds (bounds.removeFromLeft (width));
    }

private:
    AmountKnob compressor, eq, noise, preFx;
};

/*  Global drive, with a ring inside it for every distortion running, outermost first: pre, then main's slots, a stack's one
    slot once per stage, then post. Each ring is in its type's colour, with a dot where that distortion's own drive is
    once global drive has scaled it. The rings swell with every hit coming in, low or high, the middle ring as it lands and
    each one further out a little later, so it ripples outwards. Grabbing the knob sends a ripple out through them and holds
    them thicker until it's let go, both reaching each ring a little later the further out it is. */
class DriveKnob : public ParamKnob
{
public:
    explicit DriveKnob (AudioPluginAudioProcessor& p) : ParamKnob (p, "DRIVE", ParamIDs::globalDrive)
    {
        refresh.startTimerHz (15);
    }

    ~DriveKnob() override { processorRef.getScopeDataCollector().inputBandsWanted = false; }


    // worked out as it's drawn, so the dots keep up with the knob while it's turned
    void paint (juce::Graphics& g) override
    {
        rings = activeRings();

        const auto size = (float) juce::jmin (knobBounds.getWidth(), knobBounds.getHeight());
        const auto centre = knobBounds.getCentre().toFloat();

        g.setColour (theme().startDriveRing);
        g.drawEllipse (juce::Rectangle<float> (size, size).reduced (12.0f).withCentre (centre), 2.0f);

        // nested in from where a knob's thick ring usually sits, closer together the more there are
        const auto outer = size * 0.5f - 20.0f;
        const auto step = juce::jlimit (4.0f, 10.0f, outer * 0.75f / (float) juce::jmax (1, (int) rings.size()));
        const auto rotary = getRotaryParameters();

        for (size_t i = 0; i < rings.size(); ++i)
        {
            const auto radius = outer - step * (float) i;
            const auto angle = rotary.startAngleRadians + rings[i].drive * (rotary.endAngleRadians - rotary.startAngleRadians);

            const auto fromCentre = juce::jmin (hold.size() - 1, rings.size() - 1 - i);

            g.setColour (rings[i].colour);
            g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre),
                           thinnest + hold[fromCentre] * heldThickness + swelling[fromCentre] * swellThickness);

            g.setColour (theme().knobThumb);
            const auto dot = juce::jmax (2.0f, step * 0.4f);
            g.fillEllipse (juce::Rectangle<float> (dot, dot).withCentre (centre.getPointOnCircumference (radius, angle)));
        }

        drawThumb (g, knobBounds.toFloat(), (float) valueToProportionOfLength (getValue()), theme().knobThumb, 0.08f);
    }

    void startedDragging() override
    {
        held = true;
        grabbedAt = now();
        ripples.push_back (grabbedAt);
    }

    void stoppedDragging() override
    {
        held = false;
        releasedAt = now();
    }

private:
    static constexpr double ringDelay = 0.06;     // seconds for a ripple, a grab, a let go or the level to reach the next ring out
    static constexpr double easing = 0.08;        // seconds for a ring to get most of the way to held or let go
    static constexpr double swell = 0.3;          // seconds a ring spends thickening and thinning again as a ripple passes

    /*  How much a band's level has jumped above where it's been lately: its level right now against a slower average of
        it, the difference picking out a hit however loud or quiet everything is. */
    struct Onset
    {
        static constexpr float silentDb = -100.0f;
        static constexpr float fallDbPerSecond = 100.0f; // how quickly the level right now drops once a hit's passed
        static constexpr double averaging = 0.25;        // seconds the average takes to catch up
        static constexpr float gateDb = -50.0f;          // quieter than this, there's nothing to pick out
        static constexpr float hitDb = 10.0f;            // a jump this far above the average swells the rings all the way
        static constexpr double release = 0.2;           // seconds a swell takes to die away, so the rings don't flutter

        float nowDb = silentDb, averageDb = silentDb, shown = 0.0f;

        // takes the loudest since the last frame from the meter, and clears it for the next
        float next (LevelMeter& meter, double elapsed)
        {
            const auto peakDb = juce::Decibels::gainToDecibels (meter.getCurrent(), silentDb);
            meter.reset();

            nowDb = juce::jmax (peakDb, nowDb - fallDbPerSecond * (float) elapsed);
            averageDb += (nowDb - averageDb) * (float) (1.0 - std::exp (-elapsed / averaging));

            const auto hit = nowDb < gateDb ? 0.0f : juce::jlimit (0.0f, 1.0f, (nowDb - averageDb) / hitDb);
            shown = juce::jmax (hit, shown * (float) std::exp (-elapsed / release));
            return shown;
        }
    };

    Onset lows, highs;

    // at rest, then held and swollen added on top
    static constexpr float thinnest = 1.0f, heldThickness = 1.2f, swellThickness = 2.5f;

    static double now() { return juce::Time::getMillisecondCounterHiRes() * 0.001; }

    // in step with the display, and only while the start page is on screen
    void frame()
    {
        const auto time = now();
        const auto elapsed = time - lastFrame;
        const auto ease = (float) (1.0 - std::exp (-elapsed / easing));
        lastFrame = time;

        // the audio thread only splits the lows and highs out while there's a knob on screen to show them
        processorRef.getScopeDataCollector().inputBandsWanted = isShowing();

        if (! isShowing())
            return;

        // done once it's passed the outermost ring there can be
        const auto rippleLength = ringDelay * (double) (hold.size() - 1) + swell;
        ripples.erase (std::remove_if (ripples.begin(), ripples.end(), [&] (double start) { return time > start + rippleLength; }), ripples.end());

        // whichever of the lows and highs is hitting harder, kept for as long as it takes to reach the outermost ring
        auto& scope = processorRef.getScopeDataCollector();
        levels.push_back ({ time, juce::jmax (lows.next (scope.inputLows, elapsed), highs.next (scope.inputHighs, elapsed)) });
        levels.erase (levels.begin(), std::find_if (levels.begin(), levels.end(), [&] (const auto& level) { return level.first >= time - ringDelay * (double) hold.size(); }));

        auto changed = false;

        // each ring hears of the grab and the let go a little later the further it is from the middle, easing to held or
        // not, and swells with the level or a passing ripple, whichever's bigger
        for (size_t fromCentre = 0; fromCentre < hold.size(); ++fromCentre)
        {
            const auto delay = ringDelay * (double) fromCentre;
            const auto target = time >= grabbedAt + delay && (held || time < releasedAt + delay) ? 1.0f : 0.0f;

            auto newHold = hold[fromCentre] + (target - hold[fromCentre]) * ease;

            if (std::abs (target - newHold) < 0.001f)
                newHold = target;

            auto newSwelling = 0.0f;

            for (const auto start : ripples)
            {
                const auto through = (time - start - delay) / swell;

                if (through > 0.0 && through < 1.0)
                    newSwelling = juce::jmax (newSwelling, (float) std::pow (std::sin (juce::MathConstants<double>::pi * through), 2.0));
            }

            // the level as it was when it set off from the middle, to be reaching this ring now: the newest that old
            auto level = 0.0f;

            for (const auto& [at, value] : levels)
                if (at <= time - delay)
                    level = value;

            newSwelling = juce::jmax (newSwelling, level);

            changed = changed || std::abs (newHold - hold[fromCentre]) > 0.001f || std::abs (newSwelling - swelling[fromCentre]) > 0.001f;

            hold[fromCentre] = newHold;
            swelling[fromCentre] = newSwelling;
        }

        // still while there's nothing coming in and the knob's left alone
        if (changed)
            repaint();
    }

    // counting out from the middle: how held each ring is, and how swollen by the level or a ripple, both 0 to 1
    std::array<float, 8> hold {}, swelling {};
    std::vector<double> ripples; // when each ripple still going started
    std::vector<std::pair<double, float>> levels; // how hard it's hitting, and when, newest last
    double grabbedAt = 0.0, releasedAt = 0.0, lastFrame = now();
    bool held = false;
    juce::VBlankAttachment vblank { this, [this] { frame(); } };

    struct Ring
    {
        juce::Colour colour;
        float drive;

        bool operator== (const Ring& other) const { return colour == other.colour && juce::approximatelyEqual (drive, other.drive); }
    };

    std::vector<Ring> activeRings() const
    {
        // in the order of the distortion type menu
        static constexpr AccentColours Theme::* typeColours[] { &Theme::grill, &Theme::tube, &Theme::phase, &Theme::rubidium,
                                                                &Theme::tape, &Theme::slew, &Theme::waveshape, &Theme::opto };

        auto& state = processorRef.treeState;
        std::vector<Ring> found;

        auto add = [&] (SlotId slot, int copies)
        {
            const auto* enabled = dynamic_cast<juce::AudioParameterBool*> (state.getParameter (slot.enabled().getParamID()));
            const auto* type = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (slot.type().getParamID()));
            const auto layouts = EffectInfos::layoutsFor (slot.module);

            if (enabled == nullptr || ! enabled->get() || type == nullptr
                || ! juce::isPositiveAndBelow (type->getIndex(), juce::jmin (layouts.count, (int) std::size (typeColours))))
                return;

            // the first control of each type is its drive, like the band view's drive readouts
            const auto& drive = MacroParam::fetch (state, paramIdFor (slot, layouts.data[type->getIndex()]->params[0]).getParamID());

            found.insert (found.end(), (size_t) copies, { (theme().*typeColours[type->getIndex()]).main, drive.convertTo0to1 (drive.getScaled()) });
        };

        const auto routing = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (ParamIDs::mainRouting.getParamID()))->getIndex();
        const auto bandCount = dynamic_cast<juce::AudioParameterInt*> (state.getParameter (ParamIDs::bandCount.getParamID()))->get();

        add ({ ModuleId::preDistortion, 0 }, 1);

        if (routing == MainRouting::stack)
            add ({ ModuleId::main, 0 }, dynamic_cast<juce::AudioParameterInt*> (state.getParameter (ParamIDs::stackCount.getParamID()))->get());
        else
            for (int slot = 0; slot < MainRouting::maxSlots; ++slot)
                if (MainRouting::usesSlot (routing, bandCount, slot))
                    add ({ ModuleId::main, slot }, 1);

        add ({ ModuleId::postDistortion, 0 }, 1);

        return found;
    }

    // as last drawn
    std::vector<Ring> rings;

    // what's running can change from anywhere else in the plugin, the knob only draws again when it has
    juce::TimedCallback refresh { [this]
    {
        if (activeRings() != rings)
            repaint();
    } };
};

// the start page's distortion box: global drive over the four macros
class StartPanel : public Panel
{
public:
    explicit StartPanel (AudioPluginAudioProcessor& p)
        : Panel (p, "START", &Theme::start),
          drive (p),
          macro1 (p, "MACRO 1", ParamIDs::globalMacro1),
          macro2 (p, "MACRO 2", ParamIDs::globalMacro2),
          macro3 (p, "MACRO 3", ParamIDs::globalMacro3),
          macro4 (p, "MACRO 4", ParamIDs::globalMacro4)
    {
        for (auto* control : std::initializer_list<juce::Component*> { &drive, &macro1, &macro2, &macro3, &macro4 })
            addAndMakeVisible (control);
    }

    // the macros in a 2 by 2 grid of compact rows under the knob, 1 and 2 on top, the knob taking the rest
    void resized() override
    {
        auto bounds = getLocalBounds();
        auto grid = bounds.removeFromBottom (compactRowHeight * 2);

        const auto size = juce::jmin (bounds.getWidth(), bounds.getHeight());
        drive.setBounds (bounds.withSizeKeepingCentre (size, size));

        RectSlider* const left[] { &macro1, &macro3 };
        RectSlider* const right[] { &macro2, &macro4 };

        placeColumn (grid.removeFromLeft (grid.getWidth() / 2), left, 2, RectSliderType::RightJustifified);
        placeColumn (grid, right, 2, RectSliderType::LeftJustifified);
    }

private:
    DriveKnob drive;
    RectSlider macro1, macro2, macro3, macro4;
};
