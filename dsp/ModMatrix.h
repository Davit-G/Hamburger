#pragma once

#include "EffectInfos.h"

// what can modulate. saved in presets by id, so only ever added to the end
namespace ModSources
{
    enum Index { drive, macro1, macro2, mod1, mod2, mod3, keytrack, velocity, count };

    inline const char* const ids[count] { "drive", "macro1", "macro2", "mod1", "mod2", "mod3", "keytrack", "velocity" };

    // the ones with a button in the modulation box. the global ones after them, from the last note played, are only assigned from a knob's menu
    constexpr int numInBox = keytrack;

    inline int indexOf(const juce::String& id)
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
    Amounts are in the parameter's normalised range, except keytracking a frequency, which moves it an octave per octave
    from middle C at an amount of 1. A connection can flip its source to 1 - value, and smooth it with a one pole lowpass
    before it's shaped. Anything a connection leaves out reads as 0 or off. */
class ModMatrix : public juce::ChangeBroadcaster, private juce::ValueTree::Listener, private juce::AudioProcessorParameter::Listener
{
public:
    static constexpr int chunkSize = ModDest::size;
    static constexpr int maxConnections = 64;

    static inline const juce::Identifier treeId { "MODULATION" }, connectionId { "CONNECTION" }, sourceId { "source" },
                                         destId { "dest" }, amountId { "amount" }, bipolarId { "bipolar" }, skewId { "skew" },
                                         smoothId { "smooth" }, invertId { "invert" };

    explicit ModMatrix(juce::AudioProcessorValueTreeState& s) : state(s)
    {
        macros[0] = &MacroParam::fetch(state, ParamIDs::globalDrive.getParamID());

        for (int i = 0; i < ParamIDs::numGlobalMacros; ++i)
            macros[(size_t) i + 1] = &MacroParam::fetch(state, ParamIDs::globalMacros[i]->getParamID());

        for (int m = 0; m < ParamIDs::numModulators; ++m)
        {
            modulators[(size_t) m].type = dynamic_cast<juce::AudioParameterChoice*>(state.getParameter(ParamIDs::modulatorType(m).getParamID()));
            modulators[(size_t) m].type->addListener(this);
            for (int t = 0; t < ParamIDs::numModulatorToggles; ++t)
                modulators[(size_t) m].toggles[(size_t) t] = dynamic_cast<juce::AudioParameterBool*>(state.getParameter(ParamIDs::modulatorToggle(m, t).getParamID()));

            for (int p = 0; p < ParamIDs::numModulatorParams; ++p)
                modulators[(size_t) m].params[(size_t) p] = &MacroParam::fetch(state, ParamIDs::modulatorParam(m, p).getParamID());
        }

        state.state.addListener(this);
        rebuild();
    }

    ~ModMatrix() override
    {
        state.state.removeListener(this);

        for (auto& m : modulators)
            m.type->removeListener(this);
    }

    //==============================================================================
    // message thread

    juce::ValueTree getTree() { return state.state.getOrCreateChildWithName(treeId, nullptr); }

    // anything that isn't a macro, which would only be modulated after it had been read as a source
    bool canModulate(const juce::String& paramID) const
    {
        auto* param = dynamic_cast<MacroParam*>(state.getParameter(paramID));
        return param != nullptr && std::find(macros.begin() + 1, macros.end(), param) == macros.end();
    }

    void connect(int source, const juce::String& paramID)
    {
        auto tree = getTree();

        // the drive onto itself would only ever be read unmodulated
        if (! canModulate(paramID) || tree.getNumChildren() >= maxConnections || (source == ModSources::drive && state.getParameter(paramID) == macros[0]))
            return;

        for (const auto& connection : tree)
            if (connection[sourceId].toString() == ModSources::ids[source] && connection[destId].toString() == paramID)
                return;

        // swinging either side of the knob for sources that do by nature, and a whole octave per octave for keytracking
        const auto amount = pitches(source, paramID) ? 1.0f : 0.3f;
        tree.appendChild({ connectionId, { { sourceId, ModSources::ids[source] }, { destId, paramID }, { amountId, amount }, { bipolarId, isBipolarByNature(source) } } }, nullptr);
    }

    // the type parameter of the slot a parameter is in, and a bit for each of that slot's types that has it. no type for the rest
    std::pair<juce::AudioParameterChoice*, juce::uint32> usage(const juce::String& paramID) const
    {
        for (const auto& module : pluginModules)
        {
            for (int sub = 0; sub < module.slotCount; ++sub)
            {
                const SlotId slot { module.id, sub };
                const auto prefix = slot.prefix() + "_";

                if (! paramID.startsWith(prefix))
                    continue;

                const auto id = paramID.substring(prefix.length());
                juce::uint32 mask = 0, bit = 1;

                for (const auto* layout : EffectInfos::layoutsFor(module.id))
                {
                    for (const auto& descriptor : layout->params)
                        if (descriptor.id.toString() == id)
                            mask |= bit;

                    bit <<= 1;
                }

                if (mask != 0)
                    return { dynamic_cast<juce::AudioParameterChoice*>(state.getParameter(slot.type().getParamID())), mask };
            }
        }

        return { nullptr, ~0u };
    }

    // whether the type its slot is set to has the parameter, so modulating it does anything
    bool isUsed(const juce::String& paramID) const
    {
        const auto [type, mask] = usage(paramID);
        return type == nullptr || ((mask >> type->getIndex()) & 1) != 0;
    }

    // what's routed to a parameter, for the knobs to draw
    struct Shown
    {
        int index, source;
        float amount;
        bool bipolar, pitched;
    };

    std::vector<Shown> connectionsTo(const juce::String& paramID)
    {
        std::vector<Shown> found;
        const auto tree = state.state.getChildWithName(treeId);

        for (int i = 0; i < juce::jmin(tree.getNumChildren(), maxConnections); ++i)
        {
            const auto connection = tree.getChild(i);
            const auto source = ModSources::indexOf(connection[sourceId]);

            if (source >= 0 && connection[destId].toString() == paramID)
                found.push_back({ i, source, (float) connection[amountId], (bool) connection[bipolarId], pitches(source, paramID) });
        }

        return found;
    }

    // what's routed to a parameter, just the armed or selected source's while there is one, so it's clear what that source is doing
    std::vector<Shown> shownConnectionsTo(const juce::String& paramID)
    {
        auto found = connectionsTo(paramID);

        if (const auto shownSource = getShownSource(); shownSource >= 0)
            std::erase_if(found, [shownSource](const auto& connection) { return connection.source != shownSource; });

        return found;
    }

    // how far a connection is moving its parameter right now, in the parameter's normalised range
    float shownValue(int index) const { return shown[(size_t) index].load(std::memory_order_relaxed); }

    // where a source was at the end of the last chunk, 0 to 1, for drawing its history
    float latestValue(int source) const { return latest[(size_t) source].load(std::memory_order_relaxed); }

    // a connection's source after its invert, smoothing, bipolar and skew, before its amount. 0 to 1 as the scope draws it
    float shapedValue(int index) const { return shaped[(size_t) index].load(std::memory_order_relaxed); }

    // as the scope labels it: the source, then the parameter it's routed to
    juce::String connectionName(int index) const
    {
        const auto connection = state.state.getChildWithName(treeId).getChild(index);
        const auto source = ModSources::indexOf(connection[sourceId]);
        auto* param = state.getParameter(connection[destId].toString());

        return (source >= 0 ? sourceName(source) : "") + (param != nullptr ? " > " + param->getName(40) : "");
    }

    ParamIDs::ModulatorType modulatorType(int modulator) const
    {
        return (ParamIDs::ModulatorType) modulators[(size_t) modulator].type->getIndex();
    }

    /*  The source being edited in mod edit mode, -1 when it's off. While one is, dragging anything it can modulate sets
        how much that source moves it, routing it there first if it isn't already. With shift held it's the parameter
        that's dragged. */
    int getArmedSource() const { return armedSource; }

    void setArmedSource(int source)
    {
        armedSource = source;
        sendChangeMessage();
    }

    // the modulator picked in the modulation box, -1 while it shows the macros
    void setSelectedSource(int source)
    {
        selectedSource = source;
        sendChangeMessage();
    }

    // the one source whose modulation the controls draw, the armed one over the selected one, or -1 for all of them
    int getShownSource() const { return armedSource >= 0 ? armedSource : selectedSource; }

    // the connection from a source to a parameter, invalid when there isn't one
    juce::ValueTree findConnection(int source, const juce::String& paramID)
    {
        for (const auto& connection : getTree())
            if (connection[sourceId].toString() == ModSources::ids[source] && connection[destId].toString() == paramID)
                return connection;

        return {};
    }

    // what a macro's been renamed to, or its own name
    juce::String macroName(int macro) const
    {
        const auto name = state.state[ParamIDs::macroNameProperty(macro)].toString();
        return name.isNotEmpty() ? name : "MACRO " + juce::String(macro + 1);
    }

    // as it's shown: the drive, a macro by its name, a modulator by its letter in the modulation box and its type, like B: LFO
    juce::String sourceName(int source) const
    {
        if (source == ModSources::drive)
            return "DRIVE";

        if (source <= ModSources::macro2)
            return macroName(source - ModSources::macro1);

        if (source == ModSources::keytrack)
            return "KEYTRACK";

        if (source == ModSources::velocity)
            return "VELOCITY";

        const auto modulator = source - ModSources::mod1;
        const char* const types[] { "LFO", "ENV", "AUDIO", "RAND" };
        const auto type = (int) modulatorType(modulator);

        return juce::String::charToString((juce::juce_wchar) ('A' + modulator)) + ": "
             + (juce::isPositiveAndBelow(type, (int) std::size(types)) ? types[type] : "MOD");
    }

    //==============================================================================
    // audio thread

    // the host's tempo, and where it is in quarter notes at the start of the block while it's playing, for synced LFOs
    void setTempo(double newBpm, std::optional<double> newPpq)
    {
        bpm = newBpm;
        blockPpq = newPpq;
    }

    void prepare(double newSampleRate)
    {
        sampleRate = newSampleRate;

        for (auto& m : modulators)
            m.envelope = { 0.0f, 0.0f };

        for (size_t k = 0; k < macros.size(); ++k)
            macroValues[k] = macros[k]->convertTo0to1(macros[k]->get());
    }

    // the input as it comes in, a chunk of it from start with the midi still timed against the whole block
    void process(const juce::AudioBuffer<float>& input, int start, int n, const juce::MidiBuffer& midi, int oversamplingShift)
    {
        jassert(n <= chunkSize);
        takePending();

        const auto* left = input.getReadPointer(0, start);
        const auto* right = input.getReadPointer(juce::jmin(1, input.getNumChannels() - 1), start);

        // the macros ramp across the chunk to wherever they've been moved
        for (size_t k = 0; k < macros.size(); ++k)
        {
            const auto target = macros[k]->convertTo0to1(macros[k]->get());

            for (int i = 0; i < n; ++i)
                sources[k][0][i] = sources[k][1][i] = macroValues[k] + (target - macroValues[k]) * (float) (i + 1) / (float) n;

            macroValues[k] = target;
        }

        // which note was played last and how hard, from where it lands in the chunk, over all 128
        {
            auto i = 0;

            auto fillTo = [&](int end)
            {
                for (; i < end; ++i)
                {
                    sources[ModSources::keytrack][0][i] = sources[ModSources::keytrack][1][i] = (float) note / 127.0f;
                    sources[ModSources::velocity][0][i] = sources[ModSources::velocity][1][i] = velocity;
                }
            };

            const auto end = midi.findNextSamplePosition(start + n);

            for (auto it = midi.findNextSamplePosition(start); it != end; ++it)
            {
                const auto message = (*it).getMessage();

                if (! message.isNoteOn())
                    continue;

                fillTo((*it).samplePosition - start);
                note = message.getNoteNumber();
                velocity = message.getFloatVelocity();
            }

            fillTo(n);
        }

        // a modulator switched off sits at nothing, and picks up where it was when it's back on
        const auto ppq = blockPpq ? std::optional<double>(*blockPpq + start / sampleRate * bpm / 60.0) : std::nullopt;

        for (size_t m = 0; m < modulators.size(); ++m)
        {
            auto& out = sources[ModSources::mod1 + m];

            if (modulators[m].on(ParamIDs::modOn))
                modulators[m].process(out, left, right, n, sampleRate, bpm, ppq);
            else
                for (auto& channel : out)
                    std::fill(channel, channel + n, 0.0f);
        }

        for (size_t s = 0; s < latest.size(); ++s)
            latest[s].store(sources[s][0][n - 1], std::memory_order_relaxed);

        // every parameter anything's routed to, summed from scratch
        for (int d = 0; d < numDests; ++d)
        {
            auto& dest = dests[(size_t) d];
            dest.length = n;
            dest.shift = oversamplingShift;

            for (int ch = 0; ch < 2; ++ch)
            {
                std::fill(dest.add[ch], dest.add[ch] + n, 0.0f);
                std::fill(dest.octaves[ch], dest.octaves[ch] + n, 0.0f);
            }
        }

        for (int c = 0; c < activeCount; ++c)
        {
            const auto& connection = active[(size_t) c];

            if (connection.dest == nullptr)
                continue;

            // routed to a type its slot isn't set to, it's kept but does nothing
            if (connection.type != nullptr && ((connection.usedBy >> connection.type->getIndex()) & 1) == 0)
            {
                shown[(size_t) c].store(0.0f, std::memory_order_relaxed);
                continue;
            }

            const auto& source = sources[(size_t) connection.source];
            auto& dest = dests[(size_t) connection.destIndex];
            const auto exponent = std::exp2(-2.0f * connection.skew);
            const auto flip = [&](float value) { return connection.invert ? 1.0f - value : value; };
            const auto pitched = connection.pitchPower > 0.0f && connection.source == ModSources::keytrack;

            // the octaves from middle C, however many the parameter's own mapping needs to move its frequency that far
            const auto octavesPerNote = connection.amount * connection.pitchPower / 12.0f;
            const auto octavesAt = [&](float value) { return (value * 127.0f - 60.0f) * octavesPerNote; };

            // up to a second to settle, cubed so most of the range is the short end
            const auto smoothing = Modulator::timeConstant(connection.smooth * connection.smooth * connection.smooth * 1000.0f, sampleRate);

            for (int ch = 0; ch < 2; ++ch)
            {
                auto& smoothed = smoothState[(size_t) c][(size_t) ch];

                for (int i = 0; i < n; ++i)
                {
                    smoothed = flip(source[ch][i]) + smoothing * (smoothed - flip(source[ch][i]));

                    if (pitched)
                        dest.octaves[ch][i] += octavesAt(smoothed);
                    else
                        dest.add[ch][i] += shape(smoothed, connection.bipolar, connection.skew, exponent) * connection.amount;
                }
            }

            // the chunk's last sample, as a move across the parameter's range
            const auto latestSmoothed = smoothState[(size_t) c][0];
            auto last = shape(latestSmoothed, connection.bipolar, connection.skew, exponent);
            auto moved = last * connection.amount;

            if (pitched)
            {
                const auto& range = connection.dest->range;
                const auto base = connection.dest->getBase();
                moved = range.convertTo0to1(juce::jlimit(range.start, range.end, base * std::exp2(octavesAt(latestSmoothed)))) - range.convertTo0to1(base);
                last = connection.bipolar ? latestSmoothed * 2.0f - 1.0f : latestSmoothed;
            }

            shown[(size_t) c].store(moved, std::memory_order_relaxed);
            shaped[(size_t) c].store(connection.bipolar ? last * 0.5f + 0.5f : last, std::memory_order_relaxed);
        }
    }

private:
    struct Connection
    {
        int source = 0;
        MacroParam* dest = nullptr;
        int destIndex = 0;
        float amount = 0.0f, skew = 0.0f, smooth = 0.0f;
        bool bipolar = false, invert = false;
        juce::AudioParameterChoice* type = nullptr; // the slot's type, and which of them have the parameter
        juce::uint32 usedBy = ~0u;
        float pitchPower = 0.0f;
    };

    using SourceBuffer = float[2][chunkSize];

    struct Modulator
    {
        juce::AudioParameterChoice* type = nullptr;
        std::array<juce::AudioParameterBool*, ParamIDs::numModulatorToggles> toggles {};
        std::array<MacroParam*, ParamIDs::numModulatorParams> params {};
        double phase = 0.0;
        std::array<float, 2> envelope {};

        // its own settings as modulated at the end of the last chunk
        float setting(ParamIDs::ModulatorParam which) const { return params[(size_t) which]->getModulated(); }

        bool on(ParamIDs::ModulatorToggle which) const { return toggles[(size_t) which] != nullptr && toggles[(size_t) which]->get(); }

        void process(SourceBuffer& out, const float* left, const float* right, int n, double rate, double bpm, std::optional<double> ppq)
        {
            const auto gain = juce::Decibels::decibelsToGain(setting(ParamIDs::modGain));

            switch (type->getIndex())
            {
                case ParamIDs::lfo:
                {
                    const auto increment = cycleIncrement(rate, bpm, ppq);
                    const auto shape = setting(ParamIDs::modShape) * 3.0f;
                    const auto stereo = setting(ParamIDs::modStereo) / 360.0;
                    const auto down = on(ParamIDs::modSawDown);

                    for (int i = 0; i < n; ++i)
                    {
                        phase += increment;
                        phase -= std::floor(phase);

                        auto rightPhase = phase + stereo;
                        rightPhase -= std::floor(rightPhase);

                        out[0][i] = lfoShape((float) phase, shape, down);
                        out[1][i] = lfoShape((float) rightPhase, shape, down);
                    }
                    break;
                }

                case ParamIDs::envelope:
                {
                    const auto attack = timeConstant(setting(ParamIDs::modAttack), rate);
                    const auto release = timeConstant(setting(ParamIDs::modRelease), rate);
                    const float* input[] { left, right };

                    for (int ch = 0; ch < 2; ++ch)
                    {
                        auto& env = envelope[(size_t) ch];

                        for (int i = 0; i < n; ++i)
                        {
                            const auto level = std::abs(input[ch][i]) * gain;
                            env = level + (level > env ? attack : release) * (env - level);
                            out[ch][i] = juce::jmin(1.0f, env);
                        }
                    }
                    break;
                }

                case ParamIDs::audio:
                {
                    // the audio itself, clipped to -1 to 1 and moved to 0 to 1 like every other source. rectified, it's
                    // already 0 to 1, so it blends towards that as it is rather than moved up to the top half
                    const float* input[] { left, right };
                    const auto rectify = setting(ParamIDs::modRectify) * 0.01f;

                    for (int ch = 0; ch < 2; ++ch)
                    {
                        for (int i = 0; i < n; ++i)
                        {
                            const auto x = juce::jlimit(-1.0f, 1.0f, input[ch][i] * gain);
                            out[ch][i] = (0.5f + 0.5f * x) * (1.0f - rectify) + std::abs(x) * rectify;
                        }
                    }
                    break;
                }

                // a new value each cycle, held or as the far end of a smooth noise, glided towards over a share of the cycle
                case ParamIDs::random:
                {
                    const auto increment = cycleIncrement(rate, bpm, ppq);
                    const auto algorithm = juce::roundToInt(params[ParamIDs::modRandomType]->get());
                    const auto smoothing = timeConstant(setting(ParamIDs::modSmooth) * 10.0f / (float) (increment * rate), rate);

                    for (int i = 0; i < n; ++i)
                    {
                        phase += increment;

                        if (phase >= 1.0)
                        {
                            phase -= std::floor(phase);
                            nextRandom(algorithm);
                        }

                        const auto value = algorithm == ParamIDs::perlin ? perlinAt((float) phase) : held;
                        smoothedRandom = value + smoothing * (smoothedRandom - value);
                        out[0][i] = out[1][i] = smoothedRandom;
                    }
                    break;
                }

                default:
                    std::fill(out[0], out[0] + n, 0.0f);
                    std::fill(out[1], out[1] + n, 0.0f);
                    break;
            }
        }

        // a cycle's worth of phase per sample, at the tempo when synced, and from where the host is in the bar while it's playing
        double cycleIncrement(double rate, double bpm, std::optional<double> ppq)
        {
            if (! on(ParamIDs::modSync))
                return setting(on(ParamIDs::modWideRange) ? ParamIDs::modRateWide : ParamIDs::modRate) / rate;

            const auto beats = beatDivision(setting(ParamIDs::modSyncRate)).beats;

            if (ppq)
                phase = std::fmod(*ppq / beats, 1.0);

            return bpm / 60.0 / beats / rate;
        }

        void nextRandom(int algorithm)
        {
            gradients = { gradients[1], random.nextFloat() * 2.0f - 1.0f };

            if (algorithm != ParamIDs::gaussian)
            {
                held = random.nextFloat();
                return;
            }

            // Box-Muller, most of it within a third of the way either side of the middle
            const auto u = 1.0f - random.nextFloat(), v = random.nextFloat();
            const auto normal = std::sqrt(-2.0f * std::log(u)) * std::cos(juce::MathConstants<float>::twoPi * v);
            held = juce::jlimit(0.0f, 1.0f, 0.5f + 0.15f * normal);
        }

        // 1D perlin noise between this cycle's gradient and the next, which stays within half either side of the middle
        float perlinAt(float t) const
        {
            const auto fade = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
            const auto from = gradients[0] * t, to = gradients[1] * (t - 1.0f);
            return juce::jlimit(0.0f, 1.0f, 0.5f + from + fade * (to - from));
        }

        juce::Random random;
        float held = 0.5f, smoothedRandom = 0.5f;
        std::array<float, 2> gradients {};

        static float timeConstant(float ms, double rate)
        {
            // 0ms follows instantly, kept away from the division by 0 that fast math can't be trusted with
            return ms > 0.0f ? (float) std::exp(-1.0 / (ms * 0.001 * rate)) : 0.0f;
        }

        // sine, triangle, saw, square, crossfaded along shape from 0 to 3, all starting at the middle and rising, the saw falling when down
        static float lfoShape(float phase, float shape, bool sawDown)
        {
            const auto saw = phase < 0.5f ? phase * 2.0f : phase * 2.0f - 2.0f;
            const float waves[] {
                std::sin(juce::MathConstants<float>::twoPi * phase),
                phase < 0.25f ? phase * 4.0f : phase < 0.75f ? 2.0f - phase * 4.0f : phase * 4.0f - 4.0f,
                sawDown ? -saw : saw,
                phase < 0.5f ? 1.0f : -1.0f,
            };

            const auto from = juce::jmin(2, (int) shape);
            const auto blend = shape - (float) from;
            return 0.5f + 0.5f * (waves[from] + (waves[from + 1] - waves[from]) * blend);
        }
    };

    // a source's 0 to 1 as a connection takes it: to -1 to 1 when bipolar, then bent by the skew keeping its sign
    static float shape(float value, bool bipolar, float skew, float exponent)
    {
        if (bipolar)
            value = value * 2.0f - 1.0f;

        return skew != 0.0f ? std::copysign(std::pow(std::abs(value), exponent), value) : value;
    }

    bool isBipolarByNature(int source) const
    {
        if (source < ModSources::mod1 || source > ModSources::mod3)
            return false;

        const auto type = modulatorType(source - ModSources::mod1);
        return type == ParamIDs::lfo || type == ParamIDs::audio || type == ParamIDs::random;
    }

    // keytracking a frequency moves it by octaves: a hz knob directly, the frequency shifts, the phase distortion's and the pre fx's, through their cube
    float pitchPowerOf(const juce::String& paramID) const
    {
        auto* param = dynamic_cast<MacroParam*>(state.getParameter(paramID));

        if (param == nullptr)
            return 0.0f;

        const auto& descriptor = param->getDescriptor();

        if (descriptor.id == ParamIDs::phaseShift.id || descriptor.id == ParamIDs::freqShift.id)
            return 1.0f / 3.0f;

        return descriptor.unit == ValueType::hz || descriptor.id == ParamIDs::sizzleFrequency.id ? 1.0f : 0.0f;
    }

    bool pitches(int source, const juce::String& paramID) const { return source == ModSources::keytrack && pitchPowerOf(paramID) > 0.0f; }

    // the tree copied into the fixed array the audio thread takes from, whichever thread changed it
    void rebuild()
    {
        std::array<Connection, maxConnections> built {};
        const auto tree = state.state.getChildWithName(treeId);
        const auto count = juce::jmin(tree.getNumChildren(), maxConnections);

        for (int i = 0; i < count; ++i)
        {
            const auto connection = tree.getChild(i);
            const auto source = ModSources::indexOf(connection[sourceId]);
            const auto dest = connection[destId].toString();

            // left without a parameter, it's kept so the rest keep their places, but does nothing
            if (source < 0 || ! canModulate(dest))
                continue;

            const auto [type, usedBy] = usage(dest);
            built[(size_t) i] = { source, dynamic_cast<MacroParam*>(state.getParameter(dest)), 0,
                                  juce::jlimit(-1.0f, 1.0f, (float) connection[amountId]), juce::jlimit(-1.0f, 1.0f, (float) connection[skewId]),
                                  juce::jlimit(0.0f, 1.0f, (float) connection[smoothId]),
                                  (bool) connection[bipolarId], (bool) connection[invertId], type, usedBy, pitchPowerOf(dest) };
        }

        {
            const juce::SpinLock::ScopedLockType lock(pendingLock);
            pending = built;
            pendingCount = count;
        }

        pendingChanged = true;
        sendChangeMessage();
    }

    // on the audio thread, the connections the message thread last built, leaving it for the next chunk if it's mid build
    void takePending()
    {
        if (! pendingChanged.exchange(false))
            return;

        const juce::SpinLock::ScopedTryLockType lock(pendingLock);

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

            const auto found = std::find(destParams.begin(), destParams.begin() + numDests, connection.dest);
            connection.destIndex = (int) std::distance(destParams.begin(), found);

            if (found == destParams.begin() + numDests)
            {
                destParams[(size_t) numDests] = connection.dest;
                dests[(size_t) numDests++].pitched = false;
            }

            dests[(size_t) connection.destIndex].pitched |= connection.pitchPower > 0.0f && connection.source == ModSources::keytrack;
        }

        for (int d = 0; d < numDests; ++d)
            destParams[(size_t) d]->mod = &dests[(size_t) d];
    }

    bool isModulationTree(const juce::ValueTree& tree) const
    {
        return tree.hasType(treeId) || tree.hasType(connectionId);
    }

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override
    {
        if (isModulationTree(tree))
            rebuild();

        // a renamed macro, for anything listing the sources
        for (int i = 0; i < ParamIDs::numGlobalMacros; ++i)
            if (tree == state.state && property == ParamIDs::macroNameProperty(i))
                sendChangeMessage();
    }
    void valueTreeChildAdded(juce::ValueTree&, juce::ValueTree& child) override { if (isModulationTree(child)) rebuild(); }
    void valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree& child, int) override { if (isModulationTree(child)) rebuild(); }
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int, int) override { if (isModulationTree(parent)) rebuild(); }
    void valueTreeRedirected(juce::ValueTree&) override { rebuild(); }

    // a modulator switched to another type goes by a new name wherever the sources are listed
    void parameterValueChanged(int, float) override { sendChangeMessage(); }
    void parameterGestureChanged(int, bool) override {}

    juce::AudioProcessorValueTreeState& state;

    std::array<MacroParam*, 1 + ParamIDs::numGlobalMacros> macros {};
    std::array<float, 1 + ParamIDs::numGlobalMacros> macroValues {};
    std::array<Modulator, ParamIDs::numModulators> modulators {};
    int armedSource = -1, selectedSource = -1;
    int note = 60;
    float velocity = 0.0f;
    double sampleRate = 44100.0;
    double bpm = 120.0;
    std::optional<double> blockPpq;

    SourceBuffer sources[ModSources::count] {};

    std::array<Connection, maxConnections> pending {}, active {};
    int pendingCount = 0, activeCount = 0;
    juce::SpinLock pendingLock;
    std::atomic<bool> pendingChanged { false };

    std::array<ModDest, maxConnections> dests {};
    std::array<MacroParam*, maxConnections> destParams {};
    int numDests = 0;

    std::array<std::atomic<float>, maxConnections> shown {}, shaped {};
    std::array<std::array<float, 2>, maxConnections> smoothState {};
    std::array<std::atomic<float>, ModSources::count> latest {};

    static_assert (ModSources::drive == 0 && ModSources::macro2 == ParamIDs::numGlobalMacros, "the macros' sources are in the order of macros");
    static_assert (ModSources::mod3 - ModSources::mod1 + 1 == ParamIDs::numModulators);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModMatrix)
};
