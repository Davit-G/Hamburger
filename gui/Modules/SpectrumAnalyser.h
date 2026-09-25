#pragma once

#include <cmath>
#include <vector>

#include <juce_dsp/juce_dsp.h>
#include <pffft/pffft.h>

#include "ScopeConstants.h"

class SpectrumAnalyser
{
public:
    explicit SpectrumAnalyser (int fftSize = scope_constants::fftSize)
        : size (fftSize),
          numBins (fftSize / 2 + 1),
          levelScale ((float) scope_constants::fftSize / (float) fftSize),
          setup (pffft_new_setup (fftSize, PFFFT_REAL)),
          frame (static_cast<float*> (pffft_aligned_malloc (sizeof (float) * (size_t) fftSize))),
          work (static_cast<float*> (pffft_aligned_malloc (sizeof (float) * (size_t) fftSize))),
          window ((size_t) fftSize, juce::dsp::WindowingFunction<float>::hann),
          history ((size_t) fftSize, 0.0f),
          bins ((size_t) numBins, 0.0f)
    {
        jassert (setup != nullptr); // a real transform needs a multiple of 32
    }

    ~SpectrumAnalyser()
    {
        pffft_destroy_setup (setup);
        pffft_aligned_free (frame);
        pffft_aligned_free (work);
    }

    void push (const float* data, size_t numSamples)
    {
        for (size_t i = 0; i < numSamples; ++i)
        {
            history[(size_t) writePosition] = data[i];
            writePosition = (writePosition + 1) % size;
        }
    }

    void update()
    {
        // oldest first, so the window's taper lands on the ends of the stretch rather than the middle
        for (int i = 0; i < size; ++i)
            frame[i] = history[(size_t) ((writePosition + i) % size)];

        window.multiplyWithWindowingTable (frame, (size_t) size);

        // interleaved re/im per bin, except pair 0, which packs the two real ones: dc and nyquist
        pffft_transform_ordered (setup, frame, frame, work, PFFFT_FORWARD);

        for (int bin = 0; bin < numBins; ++bin)
        {
            const auto magnitude = levelScale * (bin == 0          ? std::abs (frame[0])
                                               : bin == size / 2   ? std::abs (frame[1])
                                                                   : std::hypot (frame[bin * 2], frame[bin * 2 + 1]));

            const auto db = juce::Decibels::gainToDecibels (juce::jlimit (1.0e-6f, 1.0e6f, magnitude), analysisFloorDb);
            const auto normalised = juce::jmap (db, analysisFloorDb, analysisCeilingDb, 0.0f, 1.0f);

            bins[(size_t) bin] = bins[(size_t) bin] * scope_constants::spectrumSmoothing
                               + normalised * (1.0f - scope_constants::spectrumSmoothing);
        }
    }

    // single source of truth for the x axis, everything drawn over a spectrum has to go through this
    static float freqToX (double freq, float width)
    {
        const auto clamped = juce::jlimit (scope_constants::minDrawFreq, scope_constants::maxDrawFreq, freq);

        return (float) juce::jmap (std::log10 (clamped),
                                   std::log10 (scope_constants::minDrawFreq),
                                   std::log10 (scope_constants::maxDrawFreq),
                                   0.0, (double) width);
    }

    static double xToFreq (float x, float width)
    {
        const auto logFreq = juce::jmap ((double) juce::jlimit (0.0f, width, x), 0.0, (double) width,
                                         std::log10 (scope_constants::minDrawFreq),
                                         std::log10 (scope_constants::maxDrawFreq));

        return std::pow (10.0, logFreq);
    }

    /*  The curve as one line across area, silence along its bottom edge. Tilted down towards the lows by
        a fixed amount an octave, so a typical mix reads roughly level instead of sloping off to the right.
        Bins landing within a pixel of each other are merged into their loudest, so a long window's thousands
        of high bins don't turn into thousands of path segments. */
    juce::Path makePath (juce::Rectangle<float> area, double sampleRate) const
    {
        const auto binToHz = sampleRate / (double) size;

        juce::Path path;
        bool started = false;
        float pendingX = 0.0f, pendingY = 0.0f;

        for (int bin = 0; bin < numBins; ++bin)
        {
            const auto freq = (double) bin * binToHz;

            if (freq < scope_constants::minDrawFreq)
                continue;

            const auto db = juce::jmap (bins[(size_t) bin], 0.0f, 1.0f, drawFloorDb, drawCeilingDb)
                          - tiltDbPerOctave * juce::jmax (0.0f, (float) std::log2 (tiltPivotHz / freq));

            const auto magnitude = juce::jmap (juce::jlimit (drawFloorDb, drawCeilingDb, db), drawFloorDb, drawCeilingDb, 0.0f, 1.0f);

            const auto x = area.getX() + freqToX (freq, area.getWidth());
            const auto y = area.getBottom() - magnitude * area.getHeight();

            if (started && x - pendingX < 1.0f)
            {
                pendingY = juce::jmin (pendingY, y); // higher up the screen is louder
            }
            else
            {
                if (!started)
                    path.startNewSubPath (x, y);
                else
                    path.lineTo (pendingX, pendingY);

                started = true;
                pendingX = x;
                pendingY = y;
            }

            // this bin already reached the right hand edge, anything past it is off screen
            if (freq >= scope_constants::maxDrawFreq)
                break;
        }

        if (started)
            path.lineTo (pendingX, pendingY);

        return path;
    }

    // the curve with the area under it shaded, how every spectrum on screen is drawn
    void paint (juce::Graphics& g, juce::Rectangle<float> area, double sampleRate, float thickness,
                juce::Colour lineColour, juce::Colour fillColour) const
    {
        const auto curve = makePath (area, sampleRate);

        auto fill = curve;
        fill.lineTo (area.getBottomRight());
        fill.lineTo (area.getBottomLeft());
        fill.closeSubPath();

        g.setColour (fillColour);
        g.fillPath (fill);
        g.setColour (lineColour);
        g.strokePath (curve, juce::PathStrokeType (thickness));
    }

private:
    static constexpr float analysisFloorDb = -70.0f, analysisCeilingDb = 50.0f;
    static constexpr float drawFloorDb = -70.0f, drawCeilingDb = 18.0f;
    static constexpr float tiltDbPerOctave = 4.5f;

    static constexpr double tiltPivotHz = 22050.0;

    const int size, numBins;
    const float levelScale;

    PFFFT_Setup* setup = nullptr;
    float* frame = nullptr; // pffft wants its buffers simd aligned, hence not std containers
    float* work = nullptr;

    juce::dsp::WindowingFunction<float> window;

    std::vector<float> history;
    int writePosition = 0;

    std::vector<float> bins;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumAnalyser)
};
