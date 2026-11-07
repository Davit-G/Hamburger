#pragma once

#include "juce_dsp/juce_dsp.h"

/*  Linear prediction around a distortion. whiten() takes the input's spectral envelope off, leaving its excitation at the
    input's own level, and colour() puts an envelope back. Both are lattices, gliding through their coefficients sample for
    sample, which a lattice stays stable through as long as each stays under 1. At no formant shift and full colour the two
    run on the same coefficients and the same tilt, so with nothing between them colour() hands the input back exactly.

    The analysis looks back over the last window of input every hop, a quarter of the window, so there's no latency, the
    envelope only lags a little. The window is long enough for two cycles of a 40 Hz bass under its taper, so the envelope
    doesn't wobble with the pitch, and short enough to follow a voice. A formant shift or a colour other than full puts back an envelope of its own: the
    whitening's power spectrum read at the frequencies scaled down by the shift, raised to the colour up to full, then
    fitted with as many stages again. Past full a second lattice after it puts on the same raised to however far past, so
    2 is the envelope twice, every formant twice as deep. It's fitted from a spectrum rather than worked out any other way
    because a spectrum that's never negative always fits to coefficients under 1, so neither lattice can go unstable.

    The colouring coefficients each block went through are kept as segments, one per stretch between hops, for colour()
    to replay.

    The excitation's level comes from how much the fit expects to take off, which only holds while the sound holds still
    between hops. A tone sweeping faster than the analysis follows leaves far more behind than expected, tens of dB of it
    once anything but the whitening's own envelope goes back on. So what colour() puts out is held within maxRise of the
    input's peak, followed with an instant attack and a guardRelease fall. Handed back exactly, it's never above it. */
class LinearPrediction
{
public:
    static constexpr int maxOrder = 96;
    static constexpr double windowMs = 40.0;

    struct Settings
    {
        int order = 12;
        double tilt = 0.0;          // the pre-emphasis coefficient, 0 for none
        bool followTilt = false;    // the tilt measured off the input instead, so its own slope comes off and goes back on as it was
        double formantRatio = 1.0;  // how far up the spectrum the envelope put back is scaled
        double colour = 1.0;        // how strongly it's put back: 0 flat, 1 as it was taken off, 2 twice as deep
        double narrowerHz = 0.0;    // how much narrower every formant is put back, negative wider
    };

    void prepare (double newSampleRate, int maxBlockSize, int numChannels)
    {
        sampleRate = newSampleRate;
        windowLength = juce::jmax (16, juce::roundToInt (windowMs * 0.001 * sampleRate));
        hop = juce::jmax (1, juce::roundToInt (windowLength * hopShare));

        window.resize ((size_t) windowLength);
        windowed.resize ((size_t) windowLength);

        for (int i = 0; i < windowLength; ++i)
            window[(size_t) i] = 0.5 - 0.5 * std::cos (juce::MathConstants<double>::twoPi * (i + 0.5) / windowLength);

        // a gaussian lag window widens every peak to about lagWidthHz, so no formant comes out a whistle
        for (int i = 0; i <= maxOrder; ++i)
            lagWindow[(size_t) i] = std::exp (-0.5 * std::pow (juce::MathConstants<double>::twoPi * lagWidthHz * i / sampleRate, 2.0));

        /*  Where the envelope put back is read, as midpoints each weighing as much as the width they stand for: closely up to
            where 44.1 kHz would stop, which is everything heard, and sparsely through whatever oversampling adds over it. */
        const auto heard = juce::jmin (juce::MathConstants<double>::pi, juce::MathConstants<double>::twoPi * 22050.0 / sampleRate);
        frequencies.clear();
        weights.clear();

        for (int i = 0; i < heardPoints; ++i)
        {
            frequencies.push_back (heard * (i + 0.5) / heardPoints);
            weights.push_back (heard / heardPoints);
        }

        if (heard < juce::MathConstants<double>::pi)
        {
            const auto above = juce::MathConstants<double>::pi - heard;

            for (int i = 0; i < abovePoints; ++i)
            {
                frequencies.push_back (heard + above * (i + 0.5) / abovePoints);
                weights.push_back (above / abovePoints);
            }
        }

        channels.resize ((size_t) numChannels);
        guardFall = std::exp (-1.0 / (guardRelease * sampleRate));

        for (auto& channel : channels)
        {
            channel = {};
            channel.history.assign ((size_t) windowLength, 0.0);
            channel.inputPeaks.assign ((size_t) maxBlockSize, 0.0);
            channel.segments.resize ((size_t) (maxBlockSize / hop + 2));
            channel.untilHop = hop;
        }
    }

    void whiten (juce::dsp::AudioBlock<float>& block, Settings newSettings)
    {
        newSettings.order = juce::jlimit (1, maxOrder, newSettings.order);
        const auto newOrder = newSettings.order;

        for (size_t ch = 0; ch < juce::jmin (block.getNumChannels(), channels.size()); ++ch)
        {
            auto& channel = channels[ch];
            auto* samples = block.getChannelPointer (ch);

            /*  Stages brought in start from nothing there and then, on both sides, so the two keep the same history. Stages
                left out glide to 0 over the next hop first, see analyse(). */
            if (newOrder > channel.stages)
            {
                for (auto i = (size_t) channel.stages; i < (size_t) newOrder; ++i)
                    channel.k[i] = channel.step[i] = channel.kc[i] = channel.stepc[i] = channel.kc2[i] = channel.stepc2[i] = channel.whitenState[i] = 0.0;

                channel.stages = newOrder;
            }

            channel.used = 0;

            for (size_t start = 0; start < block.getNumSamples();)
            {
                const auto length = juce::jmin (block.getNumSamples() - start, (size_t) channel.untilHop);
                channel.segments[(size_t) channel.used++] = { length, channel.stages, channel.again, channel.kc, channel.stepc, channel.kc2, channel.stepc2,
                                                              channel.correction, channel.correctionStep, channel.level, channel.levelStep,
                                                              channel.tilt, channel.tiltStep };

                for (auto n = start; n < start + length; ++n)
                {
                    const auto x = (double) samples[n];
                    channel.inputPeak = juce::jmax (std::abs (x), channel.inputPeak * guardFall);
                    channel.inputPeaks[n] = channel.inputPeak;
                    const auto tilted = x - channel.tilt * channel.emphasis;
                    channel.emphasis = x;

                    channel.history[(size_t) channel.written] = x;
                    channel.written = (channel.written + 1) % windowLength;

                    // forward and backward prediction errors up the stages, each stage's backward error kept for the next sample
                    auto forward = tilted, backward = tilted, remaining = 1.0;

                    for (int i = 0; i < channel.stages; ++i)
                    {
                        const auto k = channel.k[(size_t) i];
                        const auto before = channel.whitenState[(size_t) i];

                        channel.whitenState[(size_t) i] = backward;
                        backward = before + k * forward;
                        forward += k * before;
                        remaining *= 1.0 - k * k;
                    }

                    samples[n] = (float) (forward * gainFor (remaining) * channel.level);

                    // a sample's worth along the way to the last analysis
                    for (int i = 0; i < channel.stages; ++i)
                    {
                        channel.k[(size_t) i] += channel.step[(size_t) i];
                        channel.kc[(size_t) i] += channel.stepc[(size_t) i];
                        channel.kc2[(size_t) i] += channel.stepc2[(size_t) i];
                    }

                    channel.correction += channel.correctionStep;
                    channel.level += channel.levelStep;
                    channel.tilt += channel.tiltStep;
                }

                start += length;
                channel.untilHop -= (int) length;

                if (channel.untilHop == 0)
                {
                    analyse (channel, newSettings);
                    channel.untilHop = hop;
                }
            }
        }

    }

    // after the last whiten(), through the colouring coefficients it went through
    void colour (juce::dsp::AudioBlock<float>& block)
    {
        for (size_t ch = 0; ch < juce::jmin (block.getNumChannels(), channels.size()); ++ch)
        {
            auto& channel = channels[ch];
            auto* samples = block.getChannelPointer (ch);
            size_t n = 0;

            for (int s = 0; s < channel.used; ++s)
            {
                auto segment = channel.segments[(size_t) s];
                const auto stages = segment.stages;

                // stages coming in start from silence, as the whitening's did, and the second lattice too whenever it comes back
                for (auto i = channel.colouring; i < stages; ++i)
                    channel.colourState[(size_t) i] = channel.colourState2[(size_t) i] = 0.0;

                if (segment.again && ! channel.colouringAgain)
                    channel.colourState2 = {};

                channel.colouring = stages;
                channel.colouringAgain = segment.again;

                for (size_t i = 0; i < segment.length; ++i, ++n)
                {
                    auto forward = synthesise (segment.kc, channel.colourState, stages, (double) samples[n] / segment.level);

                    if (segment.again)
                        forward = synthesise (segment.kc2, channel.colourState2, stages, forward / segment.correction);

                    const auto untilted = forward + segment.tilt * channel.deemphasis;
                    channel.deemphasis = untilted;
                    channel.outputPeak = juce::jmax (std::abs (untilted), channel.outputPeak * guardFall);
                    const auto ceiling = maxRise * channel.inputPeaks[n];
                    samples[n] = (float) (channel.outputPeak > ceiling ? untilted * ceiling / channel.outputPeak : untilted);

                    for (int stage = 0; stage < stages; ++stage)
                    {
                        segment.kc[(size_t) stage] += segment.stepc[(size_t) stage];
                        segment.kc2[(size_t) stage] += segment.stepc2[(size_t) stage];
                    }

                    segment.correction += segment.correctionStep;
                    segment.level += segment.levelStep;
                    segment.tilt += segment.tiltStep;
                }
            }
        }
    }

private:
    static constexpr double hopShare = 0.25;
    static constexpr double lagWidthHz = 40.0;
    static constexpr double whiteNoise = 1.0e-7; // -70 dB under the input, the deepest the whitening digs, a voice's highs and all
    static constexpr double maxGain = 1.0e4;     // only a guard: the white noise keeps every fit under about 3000
    static constexpr double maxK = 0.999;
    static constexpr double maxFollowedTilt = 0.98;
    static constexpr double maxRise = 2.0;        // +6 dB, the most colour() puts out over the input's peak
    static constexpr double guardRelease = 0.05;  // seconds, how fast the peaks the guard follows fall
    static constexpr int heardPoints = 512, abovePoints = 64;

    using Coefficients = std::array<double, maxOrder>;
    using Correlation = std::array<double, maxOrder + 1>;

    // the colouring coefficients a stretch of the block started on, and how they glided through it
    struct Segment
    {
        size_t length = 0;
        int stages = 0;
        bool again = false;
        Coefficients kc {}, stepc {}, kc2 {}, stepc2 {};
        double correction = 1.0, correctionStep = 0.0, level = 1.0, levelStep = 0.0, tilt = 0.0, tiltStep = 0.0;
    };

    struct Channel
    {
        std::vector<double> history;
        int written = 0, untilHop = 0, used = 0;
        Coefficients k {}, step {}, whitenState {};   // whitening's
        Coefficients kc {}, stepc {}, colourState {}; // colouring's, the same unless the formants are shifted or recoloured
        Coefficients kc2 {}, stepc2 {}, colourState2 {}; // the second lattice's, past full colour

        // whether the second lattice runs: while it's past full, or gliding back to nothing from it
        bool again = false, againTarget = false, colouringAgain = false;
        double correction = 1.0, correctionStep = 0.0; // see cascadeCorrection()
        double level = 1.0, levelStep = 0.0;           // the input's level over the tilted input's, see analyse()
        double tilt = 0.0, tiltStep = 0.0;             // the pre-emphasis, gliding like the coefficients so both sides match

        // how many stages the glide goes through, the most of where it's from and where it's to, and where it's to
        int stages = 0, targetStages = 0;
        int colouring = 0; // how many colour() last ran
        double emphasis = 0.0, deemphasis = 0.0;
        std::vector<Segment> segments;
        double inputPeak = 0.0, outputPeak = 0.0;   // see maxRise
        std::vector<double> inputPeaks;             // the input's at every sample of the block, for colour()
    };

    // the excitation's level against the input's, from how much of it every stage left
    static double gainFor (double remaining) { return juce::jmin (maxGain, 1.0 / std::sqrt (remaining)); }

    /*  One all-pole lattice's sample, down the stages, each one's backward error for the next sample from the forward error
        under it. Brought down first by the level its envelope would've brought white noise up by, so it puts back as loud
        as it took off. */
    static double synthesise (const Coefficients& k, Coefficients& state, int stages, double x)
    {
        auto remaining = 1.0;

        for (int stage = 0; stage < stages; ++stage)
            remaining *= 1.0 - k[(size_t) stage] * k[(size_t) stage];

        auto forward = x / gainFor (remaining);

        for (int stage = stages - 1; stage >= 0; --stage)
        {
            forward -= k[(size_t) stage] * state[(size_t) stage];

            if (stage + 1 < stages)
                state[(size_t) stage + 1] = state[(size_t) stage] + k[(size_t) stage] * forward;
        }

        state[0] = forward;
        return forward;
    }

    // reflection coefficients for an autocorrelation by levinson-durbin, with the polynomial they make, all 0 for silence
    static Coefficients reflections (Correlation r, int stages, Correlation* polynomial = nullptr)
    {
        Coefficients k {};
        Correlation a {}, previous {};
        a[0] = 1.0;

        if (r[0] > 1.0e-10)
        {
            r[0] *= 1.0 + whiteNoise;
            auto error = r[0];

            for (int i = 1; i <= stages; ++i)
            {
                auto acc = r[(size_t) i];

                for (int j = 1; j < i; ++j)
                    acc += a[(size_t) j] * r[(size_t) (i - j)];

                const auto unclamped = -acc / error;
                const auto reflection = juce::jlimit (-maxK, maxK, unclamped);
                previous = a;

                for (int j = 1; j < i; ++j)
                    a[(size_t) j] = previous[(size_t) j] + reflection * previous[(size_t) (i - j)];

                a[(size_t) i] = reflection;
                k[(size_t) i - 1] = reflection;
                error *= 1.0 - reflection * reflection;

                /*  A pure tone fits in a couple of stages, after which the next wants to be 1. Clamped, the polynomial no
                    longer predicts as well as the error says, so every stage past it comes out over 1 and clamped too, all
                    of them poles on the unit circle that blow up whatever's put through. It's left at the order it got to. */
                if (std::abs (unclamped) > maxK)
                    break;
            }
        }

        if (polynomial != nullptr)
            *polynomial = a;

        return k;
    }

    // a prediction polynomial's own autocorrelation, which |A|^2 is the cosine series of
    static Correlation ownCorrelation (const Correlation& a, int stages)
    {
        Correlation own {};

        for (int lag = 0; lag <= stages; ++lag)
            for (int j = 0; j + lag <= stages; ++j)
                own[(size_t) lag] += a[(size_t) j] * a[(size_t) (j + lag)];

        return own;
    }

    // |A|^2 at the frequency whose cosine is c, walking cos (l w) = 2 cos w cos ((l - 1) w) - cos ((l - 2) w)
    static double powerAt (const Correlation& own, int stages, double c)
    {
        auto power = own[0];
        auto before = 1.0, now = c;

        for (int lag = 1; lag <= stages; ++lag)
        {
            power += 2.0 * own[(size_t) lag] * now;
            const auto next = 2.0 * c * now - before;
            before = now;
            now = next;
        }

        return juce::jmax (power, 1.0e-12);
    }

    // the autocorrelation of the whitening's power spectrum 1 / |A|^2, read at every frequency over the formant ratio and raised to power
    Correlation envelopeCorrelation (const Correlation& a, int stages, double ratio, double power) const
    {
        const auto own = ownCorrelation (a, stages);
        Correlation r {};

        for (size_t m = 0; m < frequencies.size(); ++m)
        {
            // past nyquist when shifting down, where the top of the envelope is held
            const auto read = juce::jmin (juce::MathConstants<double>::pi, frequencies[m] / ratio);
            const auto level = weights[m] * std::pow (1.0 / powerAt (own, stages, std::cos (read)), power);
            const auto c = std::cos (frequencies[m]);
            auto before = 1.0, now = c;
            r[0] += level;

            for (int lag = 1; lag <= stages; ++lag)
            {
                r[(size_t) lag] += level * now;
                const auto next = 2.0 * c * now - before;
                before = now;
                now = next;
            }
        }

        return r;
    }

    /*  How much louder white noise comes out of the two lattices one after the other than out of each brought down to its
        own level, as the second has to be brought down further. Both levels from the same frequencies, so the error in
        reading them off cancels. */
    double cascadeCorrection (const Correlation& first, const Correlation& second, int stages) const
    {
        const auto own1 = ownCorrelation (first, stages), own2 = ownCorrelation (second, stages);
        auto alone1 = 0.0, alone2 = 0.0, together = 0.0;

        for (size_t m = 0; m < frequencies.size(); ++m)
        {
            const auto c = std::cos (frequencies[m]);
            const auto p1 = 1.0 / powerAt (own1, stages, c), p2 = 1.0 / powerAt (own2, stages, c);

            alone1 += weights[m] * p1;
            alone2 += weights[m] * p2;
            together += weights[m] * p1 * p2;
        }

        return std::sqrt (together * juce::MathConstants<double>::pi / (alone1 * alone2));
    }

    // where the coefficients glide to over the next hop, from the windowed history
    void analyse (Channel& channel, const Settings& with)
    {
        for (int i = 0; i < windowLength; ++i)
            windowed[(size_t) i] = channel.history[(size_t) ((channel.written + i) % windowLength)] * window[(size_t) i];

        // the input's own, a lag further than the order for the tilt to reach
        std::array<double, maxOrder + 2> raw {};

        for (int lag = 0; lag <= with.order + 1; ++lag)
            for (int i = lag; i < windowLength; ++i)
                raw[(size_t) lag] += windowed[(size_t) i] * windowed[(size_t) (i - lag)];

        /*  Followed, the tilt is the input's own first order prediction, the most a single step back can take off: its slope,
            so the formants are fitted on their own and the shift and the colour only move them, never how the lows fall away. */
        const auto mu = with.followTilt ? (raw[0] > 1.0e-10 ? juce::jlimit (0.0, maxFollowedTilt, raw[1] / raw[0]) : 0.0) : with.tilt;
        channel.tiltStep = (mu - channel.tilt) / hop;

        // the tilted input's, x - tilt x[-1] in autocorrelations, near enough as the window would have had it
        Correlation r {};

        for (int lag = 0; lag <= with.order; ++lag)
            r[(size_t) lag] = ((1.0 + mu * mu) * raw[(size_t) lag] - mu * (raw[(size_t) std::abs (lag - 1)] + raw[(size_t) lag + 1]))
                              * lagWindow[(size_t) lag];

        /*  Tilting takes the lows off, and the excitation comes out as loud as what was whitened, so it's brought back up by
            as much: tilt only changes what's measured, not how hard the distortion's driven. */
        const auto level = r[0] > 1.0e-10 ? juce::jmin (maxGain, std::sqrt (raw[0] / r[0])) : 1.0;
        channel.levelStep = (level - channel.level) / hop;

        Correlation a {};
        const auto target = reflections (r, with.order, &a);

        /*  Every pole of the envelope moved out towards the unit circle, narrowing each formant by narrowerHz, or in, widening
            it: a pole's bandwidth is -ln (radius) * rate / pi. Past the circle the spectrum's the same as the pole's
            reflection's, and the fit always comes out stable, so narrowing only stops making them any sharper. */
        auto resonant = a;
        const auto radius = std::exp (juce::MathConstants<double>::pi * with.narrowerHz / sampleRate);

        auto scale = 1.0;

        for (int i = 1; i <= with.order; ++i)
            resonant[(size_t) i] *= (scale *= radius);

        // the envelope shifted and raised to a power, fitted, with the polynomial it fits to. silence has no envelope to shape
        auto fitted = [&] (double power, Correlation& polynomial)
        {
            polynomial = {};
            polynomial[0] = 1.0;

            if (r[0] <= 1.0e-10)
                return Coefficients {};

            auto shaped = envelopeCorrelation (resonant, with.order, with.formantRatio, power);

            for (int lag = 0; lag <= with.order; ++lag)
                shaped[(size_t) lag] *= lagWindow[(size_t) lag];

            return reflections (shaped, with.order, &polynomial);
        };

        // as it is, at no shift, no change in resonance and full colour or past it, so it's the whitening's own to the last bit
        const auto first = juce::jmin (1.0, with.colour);
        auto a1 = a, a2 = Correlation {};
        const auto targetc = with.formantRatio == 1.0 && with.narrowerHz == 0.0 && first == 1.0 ? target : fitted (first, a1);
        const auto targetc2 = with.colour > 1.0 ? fitted (with.colour - 1.0, a2) : Coefficients {};
        const auto correction = with.colour > 1.0 ? cascadeCorrection (a1, a2, with.order) : 1.0;

        channel.again = with.colour > 1.0 || channel.againTarget;
        channel.againTarget = with.colour > 1.0;

        // the stages past both where the glide's from and where it's to have long since glided to 0, and are left there
        channel.stages = juce::jmax (channel.targetStages, with.order);
        channel.targetStages = with.order;

        for (auto i = (size_t) channel.stages; i < (size_t) maxOrder; ++i)
            channel.k[i] = channel.kc[i] = channel.kc2[i] = 0.0;

        for (int i = 0; i < maxOrder; ++i)
        {
            channel.step[(size_t) i] = (target[(size_t) i] - channel.k[(size_t) i]) / hop;
            channel.stepc[(size_t) i] = (targetc[(size_t) i] - channel.kc[(size_t) i]) / hop;
            channel.stepc2[(size_t) i] = (targetc2[(size_t) i] - channel.kc2[(size_t) i]) / hop;
        }

        channel.correctionStep = (correction - channel.correction) / hop;
    }

    double sampleRate = 44100.0;
    int windowLength = 16, hop = 1;
    double guardFall = 0.0;
    std::vector<double> window, windowed;
    std::array<double, maxOrder + 1> lagWindow {};
    std::vector<double> frequencies, weights;
    std::vector<Channel> channels;
};
