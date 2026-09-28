#pragma once

#include "MacroParam.h"

// what can modulate. saved in presets by id, so only ever added to the end
namespace ModSources
{
    enum Index { drive, macro1, macro2, macro3, macro4, mod1, mod2, mod3, mod4, mod5, count };

    inline const char* const ids[count] { "drive", "macro1", "macro2", "macro3", "macro4", "mod1", "mod2", "mod3", "mod4", "mod5" };

    inline int indexOf (const juce::String& id)
    {
        for (int i = 0; i < count; ++i)
            if (id == ids[i])
                return i;

        return -1;
    }
}

/*  Many sources to many parameters. The connections live in the state tree under MODULATION, so they're saved with the
    preset, and are copied into a fixed array for the audio thread whenever they change. Every chunk of up to 64 samples
    each source fills a buffer, then each connection adds into its parameter's ModDest, which the parameter reads back
    per sample: see MacroParam::modulate. Every value a source makes is 0 to 1, a bipolar connection takes it to -1 to 1.
    Amounts are in the parameter's normalised range, except a keytracking modulator on a frequency, which moves it an
    octave per octave played at an amount of 1. */
class ModMatrix : public juce::ChangeBroadcaster, private juce::ValueTree::Listener, private juce::AudioProcessorParameter::Listener
{
public:
    static constexpr int chunkSize = ModDest::size;
    static constexpr int maxConnections = 64;

    static inline const juce::Identifier treeId { "MODULATION" }, connectionId { "CONNECTION" }, sourceId { "source" },
                                         destId { "dest" }, amountId { "amount" }, bipolarId { "bipolar" }, skewId { "skew" };

    explicit ModMatrix (juce::AudioProcessorValueTreeState& s) : state (s)
    {
        macros[0] = &MacroParam::fetch (state, ParamIDs::globalDrive.getParamID());

        for (int i = 0; i < ParamIDs::numGlobalMacros; ++i)
            macros[(size_t) i + 1] = &MacroParam::fetch (state, ParamIDs::globalMacros[i]->getParamID());

        for (int m = 0; m < ParamIDs::numModulators; ++m)
        {
            modulators[(size_t) m].type = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (ParamIDs::modulatorType (m).getParamID()));
            modulators[(size_t) m].type->addListener (this);

            for (int p = 0; p < ParamIDs::numModulatorParams; ++p)
                modulators[(size_t) m].params[(size_t) p] = &MacroParam::fetch (state, ParamIDs::modulatorParam (m, p).getParamID());
        }

        stackCount = dynamic_cast<juce::AudioParameterInt*> (state.getParameter (ParamIDs::stackCount.getParamID()));

        for (auto& dest : dests)
            dest.stage = &currentStage;

        state.state.addListener (this);
        rebuild();
    }

    ~ModMatrix() override
    {
        state.state.removeListener (this);

        for (auto& m : modulators)
            m.type->removeListener (this);
    }

    //==============================================================================
    // message thread

    juce::ValueTree getTree() { return state.state.getOrCreateChildWithName (treeId, nullptr); }

    // anything that isn't a macro, which would only be modulated after it had been read as a source
    bool canModulate (const juce::String& paramID) const
    {
        auto* param = dynamic_cast<MacroParam*> (state.getParameter (paramID));
        return param != nullptr && std::find (macros.begin() + 1, macros.end(), param) == macros.end();
    }

    void connect (int source, const juce::String& paramID)
    {
        auto tree = getTree();

        // the drive onto itself would only ever be read unmodulated
        if (! canModulate (paramID) || tree.getNumChildren() >= maxConnections || (source == ModSources::drive && state.getParameter (paramID) == macros[0]))
            return;

        for (const auto& connection : tree)
            if (connection[sourceId].toString() == ModSources::ids[source] && connection[destId].toString() == paramID)
                return;

        // swinging either side of the knob for sources that do by nature, and a whole octave per octave for keytracking
        const auto bipolar = isBipolarByNature (source);
        const auto amount = keytracks (source) && pitchPowerOf (paramID) > 0.0f ? 1.0f : 0.3f;

        tree.appendChild ({ connectionId, { { sourceId, ModSources::ids[source] }, { destId, paramID }, { amountId, amount },
                                            { bipolarId, bipolar }, { skewId, 0.0f } } }, nullptr);
    }

    // what's routed to a parameter, for the knobs to draw
    struct Shown
    {
        int index, source;
        float amount;
        bool bipolar, pitched;
    };

    std::vector<Shown> connectionsTo (const juce::String& paramID)
    {
        std::vector<Shown> found;
        const auto tree = state.state.getChildWithName (treeId);

        for (int i = 0; i < juce::jmin (tree.getNumChildren(), maxConnections); ++i)
        {
            const auto connection = tree.getChild (i);
            const auto source = ModSources::indexOf (connection[sourceId]);

            if (source >= 0 && connection[destId].toString() == paramID)
                found.push_back ({ i, source, (float) connection[amountId], (bool) connection[bipolarId], keytracks (source) && pitchPowerOf (paramID) > 0.0f });
        }

        return found;
    }

    // how far a connection is moving its parameter right now, in the parameter's normalised range
    float shownValue (int index) const { return shown[(size_t) index].load (std::memory_order_relaxed); }

    ParamIDs::ModulatorType modulatorType (int modulator) const
    {
        return (ParamIDs::ModulatorType) modulators[(size_t) modulator].type->getIndex();
    }

    // what a macro's been renamed to, or its own name
    juce::String macroName (int macro) const
    {
        const auto name = state.state[ParamIDs::macroNameProperty (macro)].toString();
        return name.isNotEmpty() ? name : "MACRO " + juce::String (macro + 1);
    }

    // as it's shown: the drive, a macro by its name, a modulator by its type and number
    juce::String sourceName (int source) const
    {
        if (source == ModSources::drive)
            return "DRIVE";

        if (source <= ModSources::macro4)
            return macroName (source - ModSources::macro1);

        const auto modulator = source - ModSources::mod1;
        const char* const types[] { "LFO", "ENV", "AUDIO", "VEL", "KEY", "STAGE" };
        const auto type = (int) modulatorType (modulator);

        return juce::String (juce::isPositiveAndBelow (type, (int) std::size (types)) ? types[type] : "MOD") + " " + juce::String (modulator + 1);
    }

    // a modulator set to spread across the stack's stages right now
    bool stages (int source) const
    {
        return source >= ModSources::mod1 && modulatorType (source - ModSources::mod1) == ParamIDs::stage;
    }

    // a modulator set to keytrack right now, which a frequency follows by octaves
    bool keytracks (int source) const
    {
        return source >= ModSources::mod1 && modulatorType (source - ModSources::mod1) == ParamIDs::keytrack;
    }

    //==============================================================================
    // audio thread

    // the stack stage about to run, 0 everywhere else, which is where a stage modulator starts
    void setStage (int stage) { currentStage = juce::jlimit (0, ModDest::maxStages - 1, stage); }

    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;

        for (auto& m : modulators)
            m.envelope = { 0.0f, 0.0f };

        for (size_t k = 0; k < macros.size(); ++k)
            macroValues[k] = macros[k]->convertTo0to1 (macros[k]->get());
    }

    // the input as it comes in, a chunk of it from start with the midi still timed against the whole block
    void process (const juce::AudioBuffer<float>& input, int start, int n, const juce::MidiBuffer& midi, int oversamplingShift)
    {
        jassert (n <= chunkSize);
        takePending();

        const auto* left = input.getReadPointer (0, start);
        const auto* right = input.getReadPointer (juce::jmin (1, input.getNumChannels() - 1), start);

        // the macros ramp across the chunk to wherever they've been moved
        for (size_t k = 0; k < macros.size(); ++k)
        {
            const auto target = macros[k]->convertTo0to1 (macros[k]->get());

            for (int i = 0; i < n; ++i)
                sources[k][0][i] = sources[k][1][i] = macroValues[k] + (target - macroValues[k]) * (float) (i + 1) / (float) n;

            macroValues[k] = target;
        }

        // the last note played, from where it lands in the chunk
        {
            auto i = 0;

            auto fillTo = [&] (int end)
            {
                for (; i < end; ++i)
                {
                    velocities[i] = velocity;
                    notes[i] = (float) note / 127.0f;
                }
            };

            const auto end = midi.findNextSamplePosition (start + n);

            for (auto it = midi.findNextSamplePosition (start); it != end; ++it)
            {
                const auto message = (*it).getMessage();

                if (! message.isNoteOn())
                    continue;

                fillTo ((*it).samplePosition - start);
                note = message.getNoteNumber();
                velocity = message.getFloatVelocity();
            }

            fillTo (n);
        }

        for (size_t m = 0; m < modulators.size(); ++m)
            modulators[m].process (sources[ModSources::mod1 + m], left, right, velocities, notes, n, sampleRate);

        // every parameter anything's routed to, summed from scratch
        for (int d = 0; d < numDests; ++d)
        {
            auto& dest = dests[(size_t) d];
            dest.length = n;
            dest.shift = oversamplingShift;

            for (int ch = 0; ch < 2; ++ch)
            {
                std::fill (dest.add[ch], dest.add[ch] + n, 0.0f);
                std::fill (dest.octaves[ch], dest.octaves[ch] + n, 0.0f);
            }

            std::fill (std::begin (dest.stageAdd), std::end (dest.stageAdd), 0.0f);
        }

        // the first stage gets the knob as it's set, the last the whole amount, and the ones between evenly spaced
        const auto numStages = stackCount != nullptr ? stackCount->get() : 1;

        for (int c = 0; c < activeCount; ++c)
        {
            const auto& connection = active[(size_t) c];

            if (connection.dest == nullptr)
                continue;

            const auto& source = sources[(size_t) connection.source];
            auto& dest = dests[(size_t) connection.destIndex];

            if (connection.pitchPower > 0.0f && keytracks (connection.source))
            {
                // the octaves from middle C, however many the frequency's own mapping needs to move it that far
                const auto scale = connection.amount * connection.pitchPower * 127.0f / 12.0f;
                const auto offset = connection.amount * connection.pitchPower * 60.0f / 12.0f;

                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < n; ++i)
                        dest.octaves[ch][i] += source[ch][i] * scale - offset;

                continue;
            }

            const auto exponent = std::exp2 (-2.0f * connection.skew);

            if (stages (connection.source))
            {
                for (int stage = 0; stage < ModDest::maxStages; ++stage)
                {
                    const auto through = numStages > 1 ? (float) juce::jmin (stage, numStages - 1) / (float) (numStages - 1) : 0.0f;
                    dest.stageAdd[stage] += shape (through, connection.bipolar, connection.skew, exponent) * connection.amount;
                }

                continue;
            }

            for (int ch = 0; ch < 2; ++ch)
            {
                for (int i = 0; i < n; ++i)
                {
                    auto value = connection.bipolar ? source[ch][i] * 2.0f - 1.0f : source[ch][i];

                    // bent towards the ends or the middle, keeping its sign
                    if (connection.skew != 0.0f)
                        value = std::copysign (std::pow (std::abs (value), exponent), value);

                    dest.add[ch][i] += value * connection.amount;
                }
            }
        }

        // the chunk's last sample, as a move across the parameter's range
        for (int c = 0; c < activeCount; ++c)
        {
            const auto& connection = active[(size_t) c];

            if (connection.dest == nullptr)
                continue;

            const auto last = sources[(size_t) connection.source][0][n - 1];
            auto moved = 0.0f;

            if (connection.pitchPower > 0.0f && keytracks (connection.source))
            {
                const auto& range = connection.dest->range;
                const auto base = connection.dest->getBase();
                const auto octaves = connection.amount * connection.pitchPower * (last * 127.0f - 60.0f) / 12.0f;
                moved = range.convertTo0to1 (juce::jlimit (range.start, range.end, base * std::exp2 (octaves))) - range.convertTo0to1 (base);
            }
            else if (stages (connection.source))
            {
                // where it has the first stage, which is where it has everything outside the stack too
                moved = shape (0.0f, connection.bipolar, connection.skew, std::exp2 (-2.0f * connection.skew)) * connection.amount;
            }
            else
            {
                auto value = connection.bipolar ? last * 2.0f - 1.0f : last;

                if (connection.skew != 0.0f)
                    value = std::copysign (std::pow (std::abs (value), std::exp2 (-2.0f * connection.skew)), value);

                moved = value * connection.amount;
            }

            shown[(size_t) c].store (moved, std::memory_order_relaxed);
        }
    }

private:
    struct Connection
    {
        int source = 0;
        MacroParam* dest = nullptr;
        int destIndex = 0;
        float amount = 0.0f, skew = 0.0f, pitchPower = 0.0f;
        bool bipolar = false;
    };

    using SourceBuffer = float[2][chunkSize];

    struct Modulator
    {
        juce::AudioParameterChoice* type = nullptr;
        std::array<MacroParam*, ParamIDs::numModulatorParams> params {};
        double phase = 0.0;
        std::array<float, 2> envelope {};

        // its own settings as modulated at the end of the last chunk
        float setting (ParamIDs::ModulatorParam which) const { return params[(size_t) which]->getModulated(); }

        void process (SourceBuffer& out, const float* left, const float* right, const float* velocities, const float* notes, int n, double rate)
        {
            const auto gain = juce::Decibels::decibelsToGain (setting (ParamIDs::modGain));

            switch (type->getIndex())
            {
                case ParamIDs::lfo:
                {
                    const auto increment = setting (ParamIDs::modRate) / rate;
                    const auto shape = setting (ParamIDs::modShape) * 3.0f;

                    for (int i = 0; i < n; ++i)
                    {
                        phase += increment;
                        phase -= std::floor (phase);
                        out[0][i] = out[1][i] = lfoShape ((float) phase, shape);
                    }
                    break;
                }

                case ParamIDs::envelope:
                {
                    const auto attack = timeConstant (setting (ParamIDs::modAttack), rate);
                    const auto release = timeConstant (setting (ParamIDs::modRelease), rate);
                    const float* input[] { left, right };

                    for (int ch = 0; ch < 2; ++ch)
                    {
                        auto& env = envelope[(size_t) ch];

                        for (int i = 0; i < n; ++i)
                        {
                            const auto level = std::abs (input[ch][i]) * gain;
                            env = level + (level > env ? attack : release) * (env - level);
                            out[ch][i] = juce::jmin (1.0f, env);
                        }
                    }
                    break;
                }

                case ParamIDs::audio:
                {
                    // the audio itself, clipped to -1 to 1 and moved to 0 to 1 like every other source. rectified, it's
                    // already 0 to 1, so it blends towards that as it is rather than moved up to the top half
                    const float* input[] { left, right };
                    const auto rectify = setting (ParamIDs::modRectify) * 0.01f;

                    for (int ch = 0; ch < 2; ++ch)
                    {
                        for (int i = 0; i < n; ++i)
                        {
                            const auto x = juce::jlimit (-1.0f, 1.0f, input[ch][i] * gain);
                            out[ch][i] = (0.5f + 0.5f * x) * (1.0f - rectify) + std::abs (x) * rectify;
                        }
                    }
                    break;
                }

                // the last note played: how hard, or which, over all 128
                case ParamIDs::velocity:
                case ParamIDs::keytrack:
                {
                    const auto* midi = type->getIndex() == ParamIDs::velocity ? velocities : notes;
                    std::copy (midi, midi + n, out[0]);
                    std::copy (midi, midi + n, out[1]);
                    break;
                }

                default:
                    std::fill (out[0], out[0] + n, 0.0f);
                    std::fill (out[1], out[1] + n, 0.0f);
                    break;
            }
        }

        static float timeConstant (float ms, double rate)
        {
            // 0ms follows instantly, kept away from the division by 0 that fast math can't be trusted with
            return ms > 0.0f ? (float) std::exp (-1.0 / (ms * 0.001 * rate)) : 0.0f;
        }

        // sine, triangle, saw, square, crossfaded along shape from 0 to 3, all starting at the middle and rising
        static float lfoShape (float phase, float shape)
        {
            const float waves[] {
                std::sin (juce::MathConstants<float>::twoPi * phase),
                phase < 0.25f ? phase * 4.0f : phase < 0.75f ? 2.0f - phase * 4.0f : phase * 4.0f - 4.0f,
                phase < 0.5f ? phase * 2.0f : phase * 2.0f - 2.0f,
                phase < 0.5f ? 1.0f : -1.0f,
            };

            const auto from = juce::jmin (2, (int) shape);
            const auto blend = shape - (float) from;
            return 0.5f + 0.5f * (waves[from] + (waves[from + 1] - waves[from]) * blend);
        }
    };

    // a source's 0 to 1 as a connection takes it: to -1 to 1 when bipolar, then bent by the skew keeping its sign
    static float shape (float value, bool bipolar, float skew, float exponent)
    {
        if (bipolar)
            value = value * 2.0f - 1.0f;

        return skew != 0.0f ? std::copysign (std::pow (std::abs (value), exponent), value) : value;
    }

    bool isBipolarByNature (int source) const
    {
        if (source < ModSources::mod1 || source > ModSources::mod5)
            return false;

        const auto type = modulatorType (source - ModSources::mod1);
        return type == ParamIDs::lfo || type == ParamIDs::audio;
    }

    // keytracking a frequency moves it by octaves: a hz knob directly, the phase distortion's shift through its cube
    float pitchPowerOf (const juce::String& paramID) const
    {
        auto* param = dynamic_cast<MacroParam*> (state.getParameter (paramID));

        if (param == nullptr)
            return 0.0f;

        if (param->getDescriptor().id == ParamIDs::phaseShift.id)
            return 1.0f / 3.0f;

        return param->getDescriptor().unit == ParamUnits::hz ? 1.0f : 0.0f;
    }

    // the tree copied into the fixed array the audio thread takes from, whichever thread changed it
    void rebuild()
    {
        std::array<Connection, maxConnections> built {};
        const auto tree = state.state.getChildWithName (treeId);
        const auto count = juce::jmin (tree.getNumChildren(), maxConnections);

        for (int i = 0; i < count; ++i)
        {
            const auto connection = tree.getChild (i);
            const auto source = ModSources::indexOf (connection[sourceId]);
            const auto dest = connection[destId].toString();

            // left without a parameter, it's kept so the rest keep their places, but does nothing
            if (source < 0 || ! canModulate (dest))
                continue;

            built[(size_t) i] = { source, dynamic_cast<MacroParam*> (state.getParameter (dest)), 0,
                                  juce::jlimit (-1.0f, 1.0f, (float) connection[amountId]), juce::jlimit (-1.0f, 1.0f, (float) connection[skewId]),
                                  pitchPowerOf (dest), (bool) connection[bipolarId] };
        }

        {
            const juce::SpinLock::ScopedLockType lock (pendingLock);
            pending = built;
            pendingCount = count;
        }

        pendingChanged = true;
        sendChangeMessage();
    }

    // on the audio thread, the connections the message thread last built, leaving it for the next chunk if it's mid build
    void takePending()
    {
        if (! pendingChanged.exchange (false))
            return;

        const juce::SpinLock::ScopedTryLockType lock (pendingLock);

        if (! lock.isLocked())
        {
            pendingChanged = true;
            return;
        }

        active = pending;
        activeCount = pendingCount;

        // each parameter gets one ModDest however many connections it has
        for (int d = 0; d < numDests; ++d)
            destParams[(size_t) d]->mod = nullptr;

        numDests = 0;

        for (int c = 0; c < activeCount; ++c)
        {
            auto& connection = active[(size_t) c];

            if (connection.dest == nullptr)
                continue;

            const auto found = std::find (destParams.begin(), destParams.begin() + numDests, connection.dest);
            connection.destIndex = (int) std::distance (destParams.begin(), found);

            if (found == destParams.begin() + numDests)
            {
                destParams[(size_t) numDests] = connection.dest;
                dests[(size_t) numDests].pitched = false;
                ++numDests;
            }

            dests[(size_t) connection.destIndex].pitched |= connection.pitchPower > 0.0f;
        }

        for (int d = 0; d < numDests; ++d)
            destParams[(size_t) d]->mod = &dests[(size_t) d];
    }

    bool isModulationTree (const juce::ValueTree& tree) const
    {
        return tree.hasType (treeId) || tree.hasType (connectionId);
    }

    void valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property) override
    {
        if (isModulationTree (tree))
            rebuild();

        // a renamed macro, for anything listing the sources
        for (int i = 0; i < ParamIDs::numGlobalMacros; ++i)
            if (tree == state.state && property == ParamIDs::macroNameProperty (i))
                sendChangeMessage();
    }
    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree& child) override { if (isModulationTree (child)) rebuild(); }
    void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree& child, int) override { if (isModulationTree (child)) rebuild(); }
    void valueTreeChildOrderChanged (juce::ValueTree& parent, int, int) override { if (isModulationTree (parent)) rebuild(); }
    void valueTreeRedirected (juce::ValueTree&) override { rebuild(); }

    // a modulator switched to or from keytracking changes how the knobs it's on draw it
    void parameterValueChanged (int, float) override { sendChangeMessage(); }
    void parameterGestureChanged (int, bool) override {}

    juce::AudioProcessorValueTreeState& state;

    std::array<MacroParam*, 1 + ParamIDs::numGlobalMacros> macros {};
    std::array<float, 1 + ParamIDs::numGlobalMacros> macroValues {};
    std::array<Modulator, ParamIDs::numModulators> modulators {};
    juce::AudioParameterInt* stackCount = nullptr;
    int currentStage = 0;
    int note = 60;
    float velocity = 0.0f;
    double sampleRate = 44100.0;

    SourceBuffer sources[ModSources::count] {};
    float velocities[chunkSize] {}, notes[chunkSize] {};

    std::array<Connection, maxConnections> pending {}, active {};
    int pendingCount = 0, activeCount = 0;
    juce::SpinLock pendingLock;
    std::atomic<bool> pendingChanged { false };

    std::array<ModDest, maxConnections> dests {};
    std::array<MacroParam*, maxConnections> destParams {};
    int numDests = 0;

    std::array<std::atomic<float>, maxConnections> shown {};

    static_assert (ModSources::drive == 0 && ModSources::macro4 == ParamIDs::numGlobalMacros, "the macros' sources are in the order of macros");
    static_assert (ModSources::mod5 - ModSources::mod1 + 1 == ParamIDs::numModulators);
    static_assert (ModDest::maxStages == ParamIDs::numBands, "a stage for every stack stage");

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModMatrix)
};
