#pragma once

/*  Every waveshape the waveshape_features tool analyses, shared with the plugin so both run the
    same curves. A shape is f(x, s): x in [-1, 1] and s its tuned amount. Every curve is peak
    normalised to 1 (see normalised()), so the analysis and the plugin both see them at the same
    level, and the plugin adds a loudness gain on top (see loudnessGain()). waveshape_features
    checks every shape stays finite and bounded over the whole input range, so run it after
    touching anything here.

    No JUCE in this file, the tool builds without it. */

#include <algorithm>
#include <bitset>
#include <cmath>
#include <vector>

// MSVC only defines these with _USE_MATH_DEFINES
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

namespace waveshapes {
    inline float sign(float xn) { return (xn > 0.f) - (xn < 0.f); }

    inline double tanhWaveShaper(double xn, double saturation) {
        return tanh(saturation * xn) / tanh(saturation);
    }

    inline double cursedSinWS(double xn, double saturation) {
        return xn * sin(xn * saturation) * 0.3;
    }

    inline double cooltansin(double xn, double sat) {
        return tanh(2.0 * xn + sin(2.0 * sat * xn) * 0.3);
    }

    inline double crazyfoldback(double x, double s) {
        return (2 * x) / (sqrt(s) * cosh(4.0 * x - 0.2 * s));
    }

    inline double squibbly(double x, double s) {
        if (x == 0.0) return 0.0;
        double shape = (0.05 * sin(2.0 * s * pow(std::fabs(x), 0.4)));
        return x + shape * waveshapes::sign(x);
    }

    inline double squablly(double x, double s) {
        return x + 0.2 * sin(s * pow(std::fabs(x), 1.0 / (0.00001 + std::fabs(x))));
    }

    inline double razortanh(double x, double s) {
        return tanh(x - fmod(x * x, 1.0 / (5.0 * s)));
    }

    inline double omegaN(double xn, double s) {
        double sv = s * 50;
        return (xn + s * tanh(sv * xn)) * (1 - s * 0.5);
    }

    inline double shredShape(double x, double c) {
        // crossover distort before shred
        if (std::fabs(x) < 0.005)
            return 0;

        return c * (floor(c * x * 10) - (0.5 * floor(c * x * 20)) + 0.25) * 0.1;
    }

    inline float bounceShape(float x, float c) {
        float interm = fmodf(std::fabs(x * c - 0.5f), 1.0f) - 0.5f;
        return sign(x) * c * 0.1f * interm * interm - 0.02f * c;
    }

    // ---------------- simple / saturation curves ----------------

    inline double hardClip(double x, double s) {
        return std::clamp(s * x, -1.0, 1.0);
    }

    inline double cubicSoftClip(double x, double s) {
        double v = std::clamp(s * x, -1.0, 1.0);
        return 1.5 * (v - v * v * v / 3.0);
    }

    inline double atanSat(double x, double s) {
        return atan(s * x) / atan(s);
    }

    inline double algebraicSat(double x, double s) {
        return s * x / sqrt(1.0 + s * s * x * x);
    }

    inline double reciprocalSat(double x, double s) {
        return x * (1.0 + s) / (1.0 + s * std::fabs(x));
    }

    inline double erfSat(double x, double s) {
        return erf(s * x) / erf(s);
    }

    inline double expSat(double x, double s) {
        return sign(x) * (1.0 - exp(-s * std::fabs(x))) / (1.0 - exp(-s));
    }

    inline double sineSat(double x, double s) {
        return sin(M_PI_2 * std::clamp(s * x, -1.0, 1.0));
    }

    inline double smoothstepSat(double x, double s) {
        double v = std::clamp(s * x, -1.0, 1.0);
        double a = std::fabs(v);
        return sign(v) * a * a * (3.0 - 2.0 * a) * 0.5 + v * 0.5;
    }

    // ---------------- tube asymmetric saturation ----------------

    inline double triodeBias(double x, double s) {
        const double bias = 0.3;
        return tanh(s * (x + bias)) - tanh(s * bias);
    }

    inline double tubeAsymTanh(double x, double s) {
        // positive half compresses harder than negative
        return x >= 0.0 ? tanh(s * x) : tanh(0.4 * s * x) / tanh(0.4 * s);
    }

    inline double gridConduction(double x, double s) {
        // grid current clamps the positive swing, negative stays mostly linear
        return x >= 0.0 ? (1.0 - exp(-2.0 * s * x)) * 0.6 : x * (1.0 - 0.2 * x * x);
    }

    inline double plateSoftplus(double x, double s) {
        // koren-ish plate curve: softplus knee, DC removed
        const double b = 0.4;
        auto sp = [](double v) { return log1p(exp(v)); };
        return (sp(s * (x + b)) - sp(s * b)) / s;
    }

    inline double warmPoly(double x, double s) {
        return tanh(s * (x + 0.35 * x * x - 0.15 * x * x * x));
    }

    inline double pentodeKnee(double x, double s) {
        return x >= 0.0 ? tanh(s * x) : -pow(std::fabs(tanh(s * x)), 1.8);
    }

    inline double asymSqrt(double x, double s) {
        return x >= 0.0 ? sqrt(std::min(s * x, 1.0)) : std::max(0.8 * s * x, -1.0);
    }

    inline double sagTube(double x, double s) {
        // bias shifts with signal level like power supply sag
        double env = std::fabs(x);
        return tanh(s * (x + 0.25 * env)) - tanh(s * 0.25 * env) * 0.5;
    }

    // ---------------- fuzz models ----------------

    inline double germaniumFuzz(double x, double s) {
        return x >= 0.0 ? 1.0 - exp(-s * x) : -0.7 * (1.0 - exp(2.5 * s * x));
    }

    inline double siliconFuzz(double x, double s) {
        return tanh(3.0 * s * (x + 0.1)) - tanh(0.3 * s);
    }

    inline double muffCascade(double x, double s) {
        return tanh(s * tanh(s * tanh(s * x)));
    }

    inline double octaveFuzz(double x, double s) {
        // full-wave rectify for the octave up, then clip
        return tanh(s * (std::fabs(x) - 0.5));
    }

    inline double halfWaveFuzz(double x, double s) {
        return tanh(s * std::max(x, 0.0)) - 0.5 * tanh(s * std::max(-x, 0.0) * 0.1);
    }

    inline double starvedFuzz(double x, double s) {
        // dying battery: sputters near zero, splats at the top
        double a = std::fabs(x);
        if (a < 0.15) return x * 0.1;
        return sign(x) * std::min(pow(a, 1.0 / s) * 1.2, 1.0);
    }

    inline double gatedFuzz(double x, double s) {
        double a = std::fabs(x);
        return a < 0.05 ? 0.0 : sign(x) * (1.0 - exp(-s * 10.0 * (a - 0.05)));
    }

    inline double squareFuzz(double x, double s) {
        return sign(x) * (1.0 - exp(-50.0 * s * std::fabs(x)));
    }

    // ---------------- crossover distortion ----------------

    inline double deadZone(double x, double t) {
        double a = std::fabs(x);
        return a < t ? 0.0 : sign(x) * (a - t) / (1.0 - t);
    }

    inline double crossoverGauss(double x, double t) {
        return x * (1.0 - exp(-(x * x) / (t * t)));
    }

    inline double crossoverCubic(double x, double t) {
        return std::fabs(x) < t ? x * x * x / (t * t) : x;
    }

    inline double crossoverStep(double x, double t) {
        // output transistors overshoot the notch instead of missing it
        return x == 0.0 ? 0.0 : (x + sign(x) * t) / (1.0 + t);
    }

    inline double classBAsym(double x, double t) {
        double top = 0.6 * t, bottom = 1.4 * t;
        if (x > top) return tanh(2.0 * (x - top));
        if (x < -bottom) return tanh(2.0 * (x + bottom));
        return 0.0;
    }

    inline double crossoverTanh(double x, double t) {
        return tanh(3.0 * deadZone(x, t));
    }

    // ---------------- heavy waveforms ----------------

    inline double sineFold(double x, double s) {
        return sin(s * x);
    }

    inline double triangleFold(double x, double s) {
        // reflect back into [-1, 1]
        double v = fmod(s * x + 1.0, 4.0);
        if (v < 0.0) v += 4.0;
        return v < 2.0 ? v - 1.0 : 3.0 - v;
    }

    inline double wrapAround(double x, double s) {
        double v = fmod(s * x + 1.0, 2.0);
        if (v < 0.0) v += 2.0;
        return v - 1.0;
    }

    inline double bitcrush(double x, double levels) {
        // with a fractional level count, rounding can land past full scale (-2 / 1.5 at x = -1)
        return std::clamp(round(x * levels) / levels, -1.0, 1.0);
    }

    inline double chebyshev5(double x, double) {
        return 16.0 * pow(x, 5) - 20.0 * pow(x, 3) + 5.0 * x;
    }

    inline double chebyshev8(double x, double) {
        double x2 = x * x;
        return 128.0 * x2 * x2 * x2 * x2 - 256.0 * x2 * x2 * x2 + 160.0 * x2 * x2 - 32.0 * x2 + 1.0;
    }

    inline double softStairs(double x, double s) {
        double v = s * x;
        return (v - sin(2.0 * M_PI * v) / (2.0 * M_PI)) / s;
    }

    inline double ringSaw(double x, double s) {
        return x * wrapAround(x, s);
    }

    inline double phaseFold(double x, double s) {
        return sin(s * x + sin(s * x));
    }

    // ---------------- super drive / insane ----------------

    inline double megaTanh(double x, double s) {
        return tanh(s * 100.0 * x);
    }

    inline double stackedGain(double x, double s) {
        double v = x;
        for (int i = 0; i < 6; i++) v = tanh(s * v + 0.05);
        return v;
    }

    inline double cubeSine(double x, double s) {
        return sin(s * x * x * x);
    }

    inline double powerCrush(double x, double s) {
        return sign(x) * pow(std::fabs(x), 1.0 / (s * 20.0));
    }

    inline double tanTanh(double x, double s) {
        return tanh(tan(s * x));
    }

    inline double logistic(double x, double r) {
        // a few iterations of the logistic map, re-centered
        double v = 0.5 * (x + 1.0);
        r = std::min(r, 4.0); // past 4 the map leaves [0, 1] and runs off to infinity
        for (int i = 0; i < 4; i++) v = r * v * (1.0 - v);
        return 2.0 * v - 1.0;
    }

    inline double fracDrive(double x, double s) {
        double v = s * x;
        return 2.0 * (v - floor(v)) - 1.0;
    }

    inline double chirpSquare(double x, double s) {
        return sign(sin(s * x * x + x));
    }

    inline double digitalScream(double x, double s) {
        return triangleFold(bitcrush(x, 6.0), s);
    }

    inline double shredFold(double x, double s) {
        return sineFold(x + shredShape(x, 1.0) * 5.0, s);
    }

    // ======================================================================
    // helpers for the shapes below
    // ======================================================================

    inline double kneeClip(double v, double k) {
        return v / pow(1.0 + pow(std::fabs(v), k), 1.0 / k);
    }

    inline double langevin(double v) {
        // coth(v) - 1/v, magnetisation curve of a transformer core
        return std::fabs(v) < 1e-4 ? v / 3.0 : 1.0 / tanh(v) - 1.0 / v;
    }

    inline double chebT(double x, int n) {
        if (n == 0) return 1.0;
        double v = std::clamp(x, -1.0, 1.0), a = 1.0, b = v;
        for (int i = 1; i < n; i++) { double c = 2.0 * v * b - a; a = b; b = c; }
        return b;
    }

    inline int toByte(double x) { return (int)std::lround(std::clamp((x + 1.0) * 127.5, 0.0, 255.0)); }
    inline double fromByte(int v) { return (v & 0xFF) / 127.5 - 1.0; }
    inline int popcount8(int v) { return (int)std::bitset<8>((unsigned long)(v & 0xFF)).count(); }

    // ---------------- analog hardware ----------------

    inline double germaniumDiodePair(double x, double s) { return sign(x) * log1p(s * std::fabs(x)) / log1p(s); }
    inline double siliconDiodePair(double x, double s) { return 0.7 * kneeClip(s * x / 0.7, 2.5); }
    inline double ledClipper(double x, double s) { return 0.9 * kneeClip(s * x / 0.9, 8.0); }
    inline double asymDiodeClip(double x, double s) { return x >= 0.0 ? 0.5 * tanh(2.0 * s * x) : tanh(s * x); }
    inline double germaniumLeakage(double x, double s) { return tanh(s * (x + 0.1 * x * x + 0.05)) - tanh(0.05 * s); }

    inline double germaniumBiasStarve(double x, double s) {
        double v = s * x - 0.3;
        return v > 0.0 ? 1.0 - exp(-v) : -0.3 * (1.0 - exp(3.0 * v));
    }

    inline double fuzzFaceTwoStage(double x, double s) { return tanh(3.0 * (germaniumFuzz(x, s) + 0.2)) - tanh(0.6); }
    inline double toneBenderMk1(double x, double s) { return x > 0.0 ? tanh(4.0 * s * x * x) : 0.8 * tanh(s * x); }
    inline double ratOpampClip(double x, double s) { return kneeClip(5.0 * s * x, 4.0); }
    inline double tubeScreamerBlend(double x, double s) { return 0.3 * x + 0.7 * kneeClip(3.0 * s * x, 2.0); }
    inline double klonCleanBlend(double x, double s) { return 0.5 * x + 0.5 * germaniumDiodePair(x, s); }
    inline double bjtCommonEmitter(double x, double s) { return 2.0 * (1.0 - exp(-s * (x + 1.0))) / (1.0 - exp(-2.0 * s)) - 1.0; }
    inline double jfetSquareLaw(double x, double s) { double v = std::clamp(s * x, -1.0, 1.0); return 0.5 * (v + 1.0) * (v + 1.0) - 1.0; }
    inline double mosfetSoftKnee(double x, double s) { return x >= 0.0 ? kneeClip(s * x, 3.0) : kneeClip(1.3 * s * x, 1.5); }
    inline double longTailedPair(double x, double s) { double t = tanh(s * x); return t + 0.1 * t * t; }
    inline double transformerCore(double x, double s) { return langevin(s * x) / langevin(s); }
    inline double transformerGrit(double x, double s) { return 0.6 * x + 0.4 * sign(x) * pow(std::fabs(x), 1.0 / s); }
    inline double tapeSaturation(double x, double s) { return atan(s * x) / atan(s) * (1.0 - 0.1 * x * x); }
    inline double tapeHeadBump(double x, double s) { return tanh(s * (x + 0.2 * sin(2.0 * M_PI * x) * exp(-3.0 * x * x))); }
    inline double tapeDropout(double x, double s) { return tanh(s * x) * (1.0 - 0.3 * pow(sin(7.0 * M_PI * x), 8)); }
    inline double opampRailClip(double x, double s) { return std::clamp(s * x, -0.8, 1.0); }
    inline double opampRoundedRails(double x, double s) { return x >= 0.0 ? 0.7 * kneeClip(s * x / 0.7, 6.0) : 0.9 * kneeClip(s * x / 0.9, 6.0); }
    inline double vcaDistortion(double x, double s) { return x - 0.05 * s * x * x * x + 0.03 * s * x * x; }

    inline double ladderStages(double x, double s) {
        double v = x;
        for (int i = 0; i < 4; i++) v = tanh(0.5 * s * v + 0.3 * x);
        return v;
    }

    inline double otaLinearised(double x, double s) { return asinh(s * x) / asinh(s); }
    inline double bbdHeadroom(double x, double s) { return std::clamp(s * x + 0.1, -1.0, 1.0) - 0.1; }
    inline double speakerStiffness(double x, double s) { return x / (1.0 + 0.5 * s * x * x); }
    inline double speakerBreakup(double x, double s) { return x / (1.0 + s * x * x) + 0.05 * sin(20.0 * x); }
    inline double rectifierSag(double x, double s) { return tanh(s * x) * (1.0 - 0.25 * std::fabs(x)); }
    inline double singleEndedClassA(double x, double s) {
        s = std::min(s, 2.25); // the negative swing grows as e^s, this keeps it under 10
        return (1.0 - exp(-s * x)) / (1.0 - exp(-s));
    }
    inline double pushPullAB(double x, double s) { return tanh(s * (x - 0.03 * tanh(30.0 * x))); }
    inline double cathodeFollower(double x, double s) { return x >= 0.0 ? x / (1.0 + 0.3 * s * x) : tanh(s * x) / s; }
    inline double gridBlocking(double x, double s) { double v = s * x; return v > 0.3 ? 0.3 + 0.1 * (v - 0.3) : std::max(v, -1.0); }
    inline double powerTubeDrive(double x, double s) { return tanh(s * x + 0.4 * s * x * std::fabs(x)); }

    inline double twoTriodeCascade(double x, double s) {
        // two inverting stages, each biased off centre
        auto stage = [s](double v) { return -tanh(s * (v + 0.2)) + tanh(0.2 * s); };
        return stage(stage(x));
    }

    inline double vacuumDiode(double x, double s) { return x > 0.0 ? pow(std::min(s * x, 1.0), 1.5) : 0.1 * x; }
    inline double seleniumRectifier(double x, double s) { return x > 0.0 ? tanh(s * x) : 0.3 * x * (1.0 - x); }
    inline double bridgeRectifier(double x, double s) { return 2.0 * std::max(std::fabs(s * x) - 0.15, 0.0) - 0.5; }
    inline double carbonMic(double x, double s) { return tanh(s * x) * (0.8 + 0.2 * cos(9.0 * x)); }
    inline double springDriver(double x, double s) { return tanh(s * x) + 0.08 * x * sin(40.0 * x); }
    inline double inductorSat(double x, double s) { return (langevin(s * (x + 0.1)) - langevin(0.1 * s)) / langevin(s); }
    inline double optoCell(double x, double s) { return x / (0.2 + 0.8 * sqrt(std::fabs(s * x))); }
    inline double varMu(double x, double s) { return tanh(s * x / (1.0 + std::fabs(x))); }
    inline double fetLimiter(double x, double s) { return kneeClip(s * x, 3.0); }
    inline double diodeRingMod(double x, double s) { return x * tanh(20.0 * sin(5.0 * s * x)); }
    inline double germaniumRingMod(double x, double s) { return x * tanh(4.0 * sin(3.0 * s * x)) + 0.1 * x * x; }
    inline double germaniumOctave(double x, double s) { return germaniumDiodePair(2.0 * std::fabs(x) - 1.0, s); }

    inline double germaniumGated(double x, double s) {
        // leaky gate below the threshold, germanium curve above
        double a = std::fabs(x);
        if (a < 0.12) return sign(x) * (a / 0.12) * (a / 0.12) * germaniumDiodePair(0.12, s);
        return germaniumDiodePair(x, s);
    }

    inline double germaniumTempDrift(double x, double s) { return germaniumDiodePair(x + 0.08, s) - germaniumDiodePair(0.08, s); }
    inline double diodeFeedbackLog(double x, double s) { return asinh(10.0 * s * x) / asinh(10.0 * s); }

    inline double zenerClamp(double x, double s) {
        double v = s * x;
        if (v > 0.5) return 0.5 + 0.1 * (v - 0.5);
        if (v < -0.3) return -0.3 + 0.1 * (v + 0.3);
        return v;
    }

    inline double neonBulbSnap(double x, double s) { double a = std::fabs(s * x); return sign(x) * (a < 0.4 ? 0.2 * a : 0.08 + 1.2 * (a - 0.4)); }
    inline double thyristorLatch(double x, double s) { double v = s * x; return v > 0.3 ? 1.0 : v < -0.3 ? -1.0 : 0.5 * v; }
    inline double tapeBiasHiss(double x, double s) { return tanh(s * x) + 0.03 * sin(60.0 * x); }
    inline double wireRecorder(double x, double s) { return tanh(s * x) * (1.0 - 0.5 * std::fabs(sin(3.0 * M_PI * x))); }

    // ---------------- digital processes ----------------

    inline double intOverflow8(double x, double s) { long v = std::lround(s * x * 127.0); return (((v + 128) % 256 + 256) % 256 - 128) / 127.0; }
    inline double intOverflow16(double x, double s) { long v = std::lround(s * x * 32767.0); return (((v + 32768) % 65536 + 65536) % 65536 - 32768) / 32767.0; }
    inline double int4Overflow(double x, double s) { long v = std::lround(s * x * 7.0); return (((v + 8) % 16 + 16) % 16 - 8) / 7.0; }
    inline double bitmaskAnd(double x, double) { return fromByte(toByte(x) & 0xAA); }
    inline double bitmaskXor(double x, double) { return fromByte(toByte(x) ^ 0x55); }
    inline double bitShiftLeft(double x, double) { return fromByte(toByte(x) << 1); }
    inline double grayCode(double x, double) { int v = toByte(x); return fromByte(v ^ (v >> 1)); }

    inline double bitReverse(double x, double) {
        int v = toByte(x), r = 0;
        for (int i = 0; i < 8; i++) r |= ((v >> i) & 1) << (7 - i);
        return fromByte(r);
    }

    inline double nibbleSwap(double x, double) { int v = toByte(x); return fromByte((v << 4) | (v >> 4)); }
    inline double msbStuck(double x, double) { return fromByte(toByte(x) | 0x40); }
    inline double bitRotMask(double x, double) { return fromByte(toByte(x) & 0xDB); }
    inline double lsbFlip(double x, double levels) { long v = std::lround(x * levels); return (double)(v ^ 1) / levels; }
    inline double popCount(double x, double) { return popcount8(toByte(x)) / 4.0 - 1.0; }
    inline double lfsrScramble(double x, double) { int v = toByte(x); return fromByte(v ^ (v << 3) ^ (v >> 2)); }

    inline double hashNoise(double x, double s) {
        unsigned h = (unsigned)toByte(x) * 2654435761u;
        h ^= h >> 13;
        return (1.0 - s) * x + s * ((h & 255) / 127.5 - 1.0);
    }

    inline double muLawCrush(double x, double levels) {
        double y = sign(x) * log1p(255.0 * std::fabs(x)) / log1p(255.0);
        double q = bitcrush(y, levels);
        return sign(q) * (pow(256.0, std::fabs(q)) - 1.0) / 255.0;
    }

    inline double aLawCompress(double x, double) {
        const double A = 87.6;
        double a = std::fabs(x);
        return sign(x) * (a < 1.0 / A ? A * a : 1.0 + log(A * a)) / (1.0 + log(A));
    }

    inline double truncQuant(double x, double s) { return trunc(x * s) / s; }
    inline double floorQuant(double x, double s) { return floor(x * s) / s; }
    inline double midRiseQuant(double x, double s) { return (floor(x * s) + 0.5) / s; }
    inline double fractionalBits(double x, double bits) { double levels = pow(2.0, bits); return round(x * levels) / levels; }
    inline double signMagnitudeError(double x, double) { return x < 0.0 ? -1.0 - x : x; }
    inline double offsetBinaryGlitch(double x, double) { return x >= 0.0 ? x - 1.0 : x + 1.0; }

    inline double float32Precision(double x, double s) {
        // float32 can only step by 2 around 2^24, so the small part gets rounded away
        const double big = 16777216.0;
        return ((double)(float)(s * x + big) - big) / s;
    }

    inline double integerDivide(double x, double s) { long v = std::lround(s * x * 100.0); return (double)(v / 7 * 7) / (100.0 * s); }
    inline double pwmComparator(double x, double s) { return sign(x - sin(7.0 * s * x)) * std::fabs(x); }
    inline double moduloGain(double x, double s) { return 2.0 * fmod(s * x, 0.5); }
    inline double wrapCrush(double x, double s) { return wrapAround(bitcrush(x, 8.0), s); }
    inline double saturatingAdd8(double x, double s) { return std::clamp(std::lround(127.0 * s * x) + 64.0 * sign(x), -128.0, 127.0) / 127.0; }
    inline double clipThenCrush(double x, double s) { return bitcrush(hardClip(x, s), 3.0); }
    inline double crushThenTanh(double x, double levels) { return tanh(3.0 * bitcrush(x, levels)); }

    inline double tableLookupNearest(double x, double) {
        static const double table[] = { -1.0, -0.4, -0.7, 0.1, -0.1, 0.6, 0.3, 1.0 };
        return table[(int)std::clamp((x + 1.0) * 4.0, 0.0, 7.0)];
    }

    inline double tableLookupLinear(double x, double) {
        static const double table[] = { -1.0, -0.2, 0.3, -0.4, 1.0 };
        double pos = std::clamp((x + 1.0) * 2.0, 0.0, 4.0);
        int i = std::min((int)pos, 3);
        return table[i] + (table[i + 1] - table[i]) * (pos - i);
    }

    inline double clippedAliasRing(double x, double s) { return hardClip(x, s) + (std::fabs(s * x) > 1.0 ? 0.1 * sin(40.0 * x) : 0.0); }

    // ---------------- fractal / recursive folding ----------------

    inline double recursiveSineFold(double x, double s) { double v = x; for (int i = 0; i < 3; i++) v = sin(s * v); return v; }
    inline double recursiveTriFold(double x, double s) { double v = x; for (int i = 0; i < 3; i++) v = triangleFold(v, s); return v; }
    inline double recursiveTanhSine(double x, double s) { double v = x; for (int i = 0; i < 4; i++) v = tanh(s * sin(M_PI * v)); return v; }

    inline double mandelbrotOrbit(double x, double iterations) {
        // real axis of the mandelbrot set: escape time outside, orbit value inside
        double c = -0.85 + 1.15 * x, z = 0.0;
        int n = (int)iterations;
        for (int i = 0; i < n; i++) {
            z = z * z + c;
            if (std::fabs(z) > 2.0) return 1.0 - 2.0 * i / n;
        }
        return 0.5 * z;
    }

    inline double juliaFold(double x, double s) { double z = 1.6 * x; for (int i = 0; i < 5; i++) z = std::clamp(z * z - 0.3 * s, -4.0, 4.0); return tanh(z); }

    inline double henonMap(double x, double a) {
        double px = x, py = 0.0;
        for (int i = 0; i < 4; i++) {
            double nx = 1.0 - a * px * px + py;
            py = 0.3 * px;
            px = std::clamp(nx, -3.0, 3.0);
        }
        return tanh(px);
    }

    inline double cantorStaircase(double x, double) {
        double u = std::clamp(0.5 * (x + 1.0), 0.0, 1.0), y = 0.0, scale = 0.5;
        for (int i = 0; i < 8; i++) {
            if (u < 1.0 / 3.0) u *= 3.0;
            else if (u > 2.0 / 3.0) { y += scale; u = 3.0 * u - 2.0; }
            else { y += scale; break; }
            scale *= 0.5;
        }
        return 2.0 * y - 1.0;
    }

    inline double devilsFold(double x, double s) { return triangleFold(cantorStaircase(x, 0.0), s); }

    inline double weierstrassShape(double x, double s) {
        double y = 0.0;
        for (int k = 0; k < 6; k++) y += pow(0.5, k) * cos(pow(3.0, k) * M_PI * s * x);
        return 0.5 * x + 0.25 * y;
    }

    inline double takagiBlancmange(double x, double) {
        double u = std::fabs(x), t = 0.0;
        for (int k = 0; k < 8; k++) { double v = ldexp(u, k); t += ldexp(std::fabs(v - round(v)), -k); }
        return sign(x) * 1.5 * t;
    }

    inline double selfSimilarStairs(double x, double s) {
        double y = s * x;
        for (int k = 1; k <= 4; k++) y -= ldexp(sin(ldexp(M_PI, k) * y), -k) / M_PI;
        return y / s;
    }

    inline double tentMap(double x, double iterations) {
        double u = 0.5 * (x + 1.0);
        for (int i = 0; i < (int)iterations; i++) u = u < 0.5 ? 2.0 * u : 2.0 - 2.0 * u;
        return 2.0 * u - 1.0;
    }

    inline double bakerMap(double x, double iterations) {
        double u = 0.5 * (x + 1.0);
        for (int i = 0; i < (int)iterations; i++) u = fmod(2.0 * u, 1.0);
        return 2.0 * u - 1.0;
    }

    inline double recursiveSaw(double x, double s) { double v = x; for (int i = 0; i < 3; i++) v = fracDrive(v, s); return v; }
    inline double feedbackPhaseFold(double x, double s) { double v = 0.0; for (int i = 0; i < 5; i++) v = sin(s * x + 0.8 * v); return v; }
    inline double negativeFeedbackTanh(double x, double s) { double v = 0.0; for (int i = 0; i < 8; i++) v = 0.5 * (v + tanh(s * (x - 0.7 * v))); return v; }
    inline double positiveFeedbackTanh(double x, double s) { double v = 0.0; for (int i = 0; i < 8; i++) v = tanh(s * (x + 0.9 * v)); return v; }
    inline double nestedChebyshev3(double x, double) { return chebT(chebT(x, 3), 3); }
    inline double chebyshevChain(double x, double) { return chebT(chebT(chebT(x, 2), 3), 2); }
    inline double repeatedRectify(double x, double iterations) { double y = x; for (int i = 0; i < (int)iterations; i++) y = 2.0 * std::fabs(y) - 1.0; return y; }
    inline double goldenFold(double x, double s) { double y = x; for (int i = 0; i < 5; i++) y = sin(1.618 * s * y + y); return y; }

    inline double dragonCurveBits(double x, double) {
        // paper folding sequence: turn direction from the bit above the lowest set bit
        int n = toByte(x) + 1;
        while ((n & 1) == 0) n >>= 1;
        return x * ((n & 2) == 0 ? 1.0 : -0.5);
    }

    inline double thueMorse(double x, double) { return std::fabs(x) * (popcount8(toByte(x)) % 2 ? 1.0 : -1.0); }

    inline double collatzSteps(double x, double) {
        long n = toByte(x) + 1;
        int steps = 0;
        while (n != 1 && steps < 200) { n = n % 2 ? 3 * n + 1 : n / 2; steps++; }
        return 0.5 * x + 0.5 * (steps / 64.0 - 1.0);
    }

    inline double fibonacciWord(double x, double) {
        const double phi = 1.6180339887;
        int n = toByte(x);
        int bit = (int)floor((n + 2) / phi) - (int)floor((n + 1) / phi);
        return x * (bit ? 1.0 : 0.3);
    }

    inline double continuedFraction(double x, double s) { double y = 1.0; for (int i = 0; i < 5; i++) y = 1.0 / (1.0 + s * std::fabs(x) * y); return sign(x) * (1.0 - y); }
    inline double nestedAtan(double x, double s) { double y = x; for (int i = 0; i < 4; i++) y = atan(s * M_PI * y) / M_PI_2; return y; }
    inline double logSpiralFold(double x, double s) { return x * cos(s * log1p(20.0 * std::fabs(x))); }
    inline double fractalComb(double x, double s) { double y = 0.0; for (int k = 1; k <= 6; k++) y += sin(ldexp(s * x, k)) / ldexp(1.0, k); return y; }
    inline double recursiveCrushFold(double x, double s) { double y = x; for (int i = 0; i < 3; i++) y = triangleFold(bitcrush(y, 5.0), s); return y; }

    inline double duffingSpring(double x, double s) {
        // hardening spring: solve v^3 + v = s * x with newton
        double a = s * x, v = a;
        for (int i = 0; i < 8; i++) v -= (v * v * v + v - a) / (3.0 * v * v + 1.0);
        return v;
    }

    inline double recursiveRingSaw(double x, double s) {
        double v = x;
        for (int i = 0; i < 3; i++) v = 2.0 * ringSaw(v, s);
        return tanh(v); // the doubling runs up to ~7, and where that peak lands jumps around with s
    }
    inline double lSystemFold(double x, double s) { double y = x; for (int i = 0; i < 3; i++) y = sin(M_PI * y) * cos(0.5 * M_PI * s * y); return y; }
    inline double sierpinskiMask(double x, double) { int v = toByte(x); return x * ((v & (v >> 1) & (v >> 2)) ? -1.0 : 1.0); }

    inline double ifsBranchFold(double x, double s) {
        // two maps picked by sign, iterated like an iterated function system
        double y = x;
        for (int i = 0; i < 4; i++) y = y > 0.0 ? 0.5 * y + 0.5 * sin(s * y) : 0.6 * y - 0.4 * tanh(s * y);
        return y;
    }

    // ---------------- inspired by minimal audio rift ----------------

    inline double splitFold(double x, double s) { return x >= 0.0 ? sin(M_PI_2 * s * x) : tanh(s * x); }
    inline double phaseWarp(double x, double s) { return sin(M_PI_2 * x + 0.5 * s * sin(M_PI * x)); }
    inline double grind(double x, double s) { return bitcrush(tanh(s * x), 12.0) + 0.1 * sign(x); }
    inline double meltdown(double x, double s) { return tanh(s * x) * cos(6.0 * s * x * x); }
    inline double harmonicSwirl(double x, double s) { return tanh(s * (0.5 * chebT(x, 1) + 0.3 * chebT(x, 3) - 0.2 * chebT(x, 5) + 0.1 * chebT(x, 7))); }
    inline double doubleFold(double x, double s) { return triangleFold(triangleFold(x, s), 0.5 * s + 1.0); }
    inline double smearClip(double x, double s) { return kneeClip(s * x + 0.3 * sin(3.0 * s * x), 3.0); }
    inline double stepFold(double x, double s) { return softStairs(sineFold(x, s), 3.0); }
    inline double resonant(double x, double s) { return x + 0.4 * sin(8.0 * s * x) * exp(-2.0 * std::fabs(x)); }
    inline double asymWrap(double x, double s) { return x >= 0.0 ? wrapAround(x, s) : wrapAround(x, 0.5 * s); }
    inline double crushFold(double x, double s) { return triangleFold(bitcrush(x, 7.0), s); }
    inline double octaveSplit(double x, double s) { return 0.6 * tanh(s * x) + 0.4 * (2.0 * std::fabs(x) - 1.0); }
    inline double feedbackScream(double x, double s) { double v = 0.0; for (int i = 0; i < 6; i++) v = sin(3.0 * s * x + 1.3 * v); return v; }
    inline double rubberBand(double x, double s) { return x * (1.0 + 0.5 * s * sin(4.0 * M_PI * x)) / (1.0 + 0.5 * s); }

    inline double chaosFold(double x, double iterations) {
        double r = 3.5 + 0.4 * std::fabs(x), v = 0.5 + 0.4 * x;
        for (int i = 0; i < (int)iterations; i++) v = r * v * (1.0 - v);
        return 2.0 * v - 1.0;
    }

    inline double pulseWidth(double x, double s) { return sign(sin(M_PI * s * x + 0.5)) * sqrt(std::fabs(x)); }
    inline double slopeFold(double x, double s) { return sin(4.0 * s * x * std::fabs(x)); }
    inline double spectralTilt(double x, double s) { return x + 0.3 * sin(2.0 * M_PI * s * x) * x * x; }
    inline double tubeFold(double x, double s) { return sineFold(tubeAsymTanh(x, 3.0), s); }
    inline double glassShatter(double x, double s) { return triangleFold(s * x + 0.05 * sin(80.0 * x), 1.0); }
    inline double squashStretch(double x, double) { double a = std::fabs(x); return sign(x) * pow(a, 0.3 + 0.7 * a); }
    inline double bitFold(double x, double s) { return triangleFold(intOverflow8(x, 1.0), s); }
    inline double envelopeFold(double x, double s) { return triangleFold(x * (1.0 + s * std::fabs(x)), 1.0); }
    inline double sawFold(double x, double s) { return fracDrive(sineFold(x, s), 1.5); }
    inline double combFold(double x, double s) { return 0.5 * (sineFold(x, s) + sineFold(x, 1.5 * s)); }

    // ---------------- inspired by output thermal ----------------

    inline double heatCore(double x, double s) { return 0.7 * tanh(s * x) * exp(-0.3 * s * x * x) + 0.3 * x; }
    inline double glowTube(double x, double s) { return tanh(s * (x + 0.2 * x * x)) - 0.2 * tanh(0.2 * s); }
    inline double meltRectify(double x, double s) { return 0.7 * x + 0.3 * (2.0 * std::fabs(tanh(s * x)) - 1.0); }
    inline double ember(double x, double s) { return kneeClip(s * x, 1.2); }
    inline double smolder(double x, double s) { return tanh(s * x) / (1.0 + 0.3 * s * (1.0 - cos(M_PI * x))); }
    inline double blaze(double x, double s) { return x >= 0.0 ? tanh(20.0 * s * x) : tanh(5.0 * s * x); }
    inline double scorch(double x, double s) { return triangleFold(2.5 * tanh(s * x), 1.0); }
    inline double flashover(double x, double s) { return tanh(s * x) + (std::fabs(x) > 0.7 ? 0.4 * sign(x) : 0.0); }
    inline double coolant(double x, double s) { return x - 0.3 * sin(M_PI * x) * s / (1.0 + s); }
    inline double convection(double x, double s) { return sin(M_PI_2 * x) * cos(s * x * x); }
    inline double radiant(double x, double s) { return sign(x) * (1.0 - pow(1.0 - std::min(std::fabs(x), 1.0), s)); }
    inline double inferno(double x, double s) { double v = x; for (int i = 0; i < 3; i++) v = sin(s * v) + 0.5 * tanh(3.0 * v); return v; }
    inline double plasma(double x, double s) { return 0.6 * tanh(s * x) + 0.4 * sign(x) * sin(9.0 * s * x * x); }
    inline double furnaceBias(double x, double s) { return tanh(s * (x + 0.4)) - tanh(0.4 * s) - 0.2 * x; }
    inline double heatSink(double x, double s) { return x * (1.0 + s) / (1.0 + s * x * x); }
    inline double thermistor(double x, double s) { return x * (1.0 + 0.5 * s * std::fabs(x)) / (1.0 + 0.5 * s); }
    inline double boil(double x, double s) { return x + 0.15 * std::fabs(x) * sin(30.0 * s * x); }
    inline double vapor(double x, double s) { return softStairs(tanh(s * x), 6.0); }
    inline double fission(double x, double s) { return fracDrive(tanh(s * x), 2.5); }
    inline double exhaust(double x, double s) { return gridConduction(x + 0.1, s) - gridConduction(0.1, s); }
    inline double charcoal(double x, double s) { return germaniumDiodePair(x, s) * (0.9 + 0.1 * sin(40.0 * x)); }
    inline double magma(double x, double s) { return sign(x) * log1p(10.0 * s * x * x) / log1p(10.0 * s); }
    inline double spark(double x, double s) { double d = std::fabs(x) - 0.5; return x + 0.5 * s * sign(x) * exp(-40.0 * d * d); }
    inline double afterglow(double x, double s) { return 0.5 * (tanh(s * x) + atan(3.0 * s * x) / atan(3.0 * s)); }
    inline double runaway(double x, double s) { return tanh(s * tan(1.4 * x)); }

    // ---------------- inspired by izotope trash 2 ----------------

    inline double scream(double x, double s) { return tanh(20.0 * s * x + 5.0 * sin(10.0 * s * x)); }
    inline double brokenSpeaker(double x, double s) { double a = std::fabs(x); return x / (1.0 + s * x * x) + (a > 0.6 ? 0.2 * sign(x) * (a - 0.6) : 0.0); }
    inline double buzzBox(double x, double s) { return sign(x) * pow(std::fabs(tanh(s * x)), 0.2); }
    inline double retroTransistor(double x, double s) { return tanh(s * x) * (1.0 - 0.3 * std::fabs(x)) + 0.1 * x * x; }
    inline double rectifyFull(double x, double s) { return 2.0 * std::fabs(tanh(s * x)) - 1.0; }
    inline double rectifyHalf(double x, double s) { return 2.0 * tanh(s * std::max(x, 0.0)) - 1.0; }
    inline double gnarl(double x, double s) { return x * sin(10.0 * s * std::fabs(x)); }
    inline double spikeFold(double x, double s) { return sign(x) * triangleFold(std::fabs(x), s); }
    inline double mangle(double x, double s) { return bitcrush(sineFold(x, s), 5.0); }
    inline double blownAmp(double x, double s) { return std::clamp(s * x, -0.4, 0.9) + 0.1 * sin(50.0 * x); }
    inline double tapeEater(double x, double s) { return x * (std::fabs(sin(12.0 * s * x)) > 0.2 ? 1.0 : 0.1); }
    inline double nuke(double x, double s) { return std::fabs(x) > 0.01 ? tanh(1000.0 * s * x) : 0.0; }
    inline double fryer(double x, double s) { return sign(x) * (0.5 + 0.5 * std::fabs(sin(16.0 * s * x))); }
    inline double warmth(double x, double s) { double t = tanh(s * x); return 0.8 * t + 0.2 * t * t; }
    inline double megaFold(double x, double s) { return sin(M_PI * triangleFold(x, 6.0 * s)); }
    inline double sputter(double x, double s) { return std::fabs(x) < 0.3 ? bitcrush(x, 3.0) : tanh(s * x); }
    inline double decimator(double x, double levels) { return bitcrush(x, levels); }
    inline double crossRing(double x, double s) { return crossoverGauss(x, 0.2) * sign(sin(5.0 * s * x)); }
    inline double hollow(double x, double s) { return x - 0.9 * tanh(s * x) / tanh(s); }
    inline double fuzzBite(double x, double s) { return sign(x) * (1.0 - exp(-s * std::fabs(x))) + 0.2 * x * x; }
    inline double subRumble(double x, double) { return sin(M_PI_2 * x) + 0.3 * (2.0 * x * x - 1.0); }
    inline double bitScream(double x, double s) { return wrapAround(3.0 * tanh(s * x), 1.0); }
    inline double rattle(double x, double s) { return x + 0.2 * sign(sin(40.0 * s * x)) * x * x; }
    inline double crunchStomp(double x, double s) { return tanh(3.0 * s * x + 0.5) - tanh(0.5); }
    inline double wreck(double x, double s) { return tanh(s * tan(1.2 * x)); }
    inline double squeak(double x, double s) { return sin(20.0 * s * x * x * x + x); }

    // ======================================================================
    // the table
    // ======================================================================

    enum Group { Analog, Digital, Folding, Heavy, GroupCount };
    inline constexpr const char* groupNames[GroupCount] = { "analog / asymmetric / tube", "digital / bit", "wavefolding", "heavy / fuzz / crazy" };

    struct Shape {
        const char* name;
        double (*fn)(double x, double s); // raw curve, not normalised
        double s; // tuned amount
        Group group;
        double gain = 1.0; // 1 / peak |fn| over x in [-1, 1], filled in by normalised()

        // peak normalised to 1
        double operator()(double x) const { return fn(x, s) * gain; }
    };

    /*  Fills in every shape's gain from its peak |fn| over x in [-1, 1], sampled.
        ponytail: 1025 samples, so a spike narrower than ~0.002 can slip between them. waveshape_features
        holds every normalised curve under 1.5 on a 200x finer grid, a denser peak search if one ever fails */
    inline std::vector<Shape> normalised(std::vector<Shape> list) {
        for (auto& shape : list) {
            double peak = 0.0;
            for (int i = -512; i <= 512; i++)
                peak = std::max(peak, std::fabs(shape.fn(i / 512.0, shape.s)));
            shape.gain = peak > 1e-9 ? 1.0 / peak : 1.0;
        }
        return list;
    }

    #define SHAPE(function, amount, group) {#function, function, amount, group}

    inline const std::vector<Shape> shapes = normalised({
        SHAPE(tanhWaveShaper, 2.0, Analog),
        {"bounceShape", [](double x, double s) { return (double)bounceShape((float)x, (float)s); }, 1.0, Folding},
        SHAPE(shredShape, 1.0, Digital),
        SHAPE(cursedSinWS, 1.0, Folding),
        SHAPE(omegaN, 1.0, Analog),
        SHAPE(cooltansin, 6.0, Analog),
        SHAPE(crazyfoldback, 3.0, Folding),
        SHAPE(squibbly, 8.0, Analog),
        SHAPE(squablly, 10.0, Heavy),
        SHAPE(razortanh, 3.0, Heavy),

        // simple / saturation
        SHAPE(hardClip, 1.5, Analog),
        SHAPE(cubicSoftClip, 1.5, Analog),
        SHAPE(atanSat, 3.0, Analog),
        SHAPE(algebraicSat, 2.0, Analog),
        SHAPE(reciprocalSat, 3.0, Analog),
        SHAPE(erfSat, 2.0, Analog),
        SHAPE(expSat, 3.0, Analog),
        SHAPE(sineSat, 1.2, Analog),
        SHAPE(smoothstepSat, 1.5, Analog),

        // tube asymmetric
        SHAPE(triodeBias, 2.5, Analog),
        SHAPE(tubeAsymTanh, 3.0, Analog),
        SHAPE(gridConduction, 2.0, Analog),
        SHAPE(plateSoftplus, 4.0, Analog),
        SHAPE(warmPoly, 1.5, Analog),
        SHAPE(pentodeKnee, 2.0, Analog),
        SHAPE(asymSqrt, 1.5, Analog),
        SHAPE(sagTube, 3.0, Analog),

        // fuzz
        SHAPE(germaniumFuzz, 6.0, Heavy),
        SHAPE(siliconFuzz, 5.0, Heavy),
        SHAPE(muffCascade, 4.0, Heavy),
        SHAPE(octaveFuzz, 6.0, Heavy),
        SHAPE(halfWaveFuzz, 8.0, Heavy),
        SHAPE(starvedFuzz, 4.0, Heavy),
        SHAPE(gatedFuzz, 2.0, Heavy),
        SHAPE(squareFuzz, 1.0, Heavy),

        // crossover
        SHAPE(deadZone, 0.2, Analog),
        SHAPE(crossoverGauss, 0.3, Analog),
        SHAPE(crossoverCubic, 0.4, Analog),
        SHAPE(crossoverStep, 0.2, Analog),
        SHAPE(classBAsym, 0.2, Analog),
        SHAPE(crossoverTanh, 0.15, Analog),

        // heavy waveforms
        SHAPE(sineFold, 6.0, Folding),
        SHAPE(triangleFold, 4.0, Folding),
        SHAPE(wrapAround, 3.0, Folding),
        SHAPE(bitcrush, 4.0, Digital),
        SHAPE(chebyshev5, 0.0, Folding),
        SHAPE(chebyshev8, 0.0, Folding),
        SHAPE(softStairs, 4.0, Folding),
        SHAPE(ringSaw, 5.0, Folding),
        SHAPE(phaseFold, 4.0, Folding),

        // super drive / insane
        SHAPE(megaTanh, 1.0, Heavy),
        SHAPE(stackedGain, 4.0, Heavy),
        SHAPE(cubeSine, 30.0, Folding),
        SHAPE(powerCrush, 1.0, Heavy),
        SHAPE(tanTanh, 5.0, Heavy),
        SHAPE(logistic, 3.9, Heavy),
        SHAPE(fracDrive, 7.0, Folding),
        SHAPE(chirpSquare, 40.0, Heavy),
        SHAPE(digitalScream, 5.0, Digital),
        SHAPE(shredFold, 3.0, Folding),

        // analog hardware
        SHAPE(germaniumDiodePair, 8.0, Analog),
        SHAPE(siliconDiodePair, 3.0, Analog),
        SHAPE(ledClipper, 2.0, Analog),
        SHAPE(asymDiodeClip, 3.0, Analog),
        SHAPE(germaniumLeakage, 3.0, Analog),
        SHAPE(germaniumBiasStarve, 3.0, Analog),
        SHAPE(fuzzFaceTwoStage, 6.0, Heavy),
        SHAPE(toneBenderMk1, 3.0, Heavy),
        SHAPE(ratOpampClip, 2.0, Analog),
        SHAPE(tubeScreamerBlend, 2.0, Analog),
        SHAPE(klonCleanBlend, 10.0, Analog),
        SHAPE(bjtCommonEmitter, 2.0, Analog),
        SHAPE(jfetSquareLaw, 1.0, Analog),
        SHAPE(mosfetSoftKnee, 3.0, Analog),
        SHAPE(longTailedPair, 3.0, Analog),
        SHAPE(transformerCore, 4.0, Analog),
        SHAPE(transformerGrit, 3.0, Analog),
        SHAPE(tapeSaturation, 2.0, Analog),
        SHAPE(tapeHeadBump, 2.0, Analog),
        SHAPE(tapeDropout, 2.0, Analog),
        SHAPE(opampRailClip, 2.0, Analog),
        SHAPE(opampRoundedRails, 2.0, Analog),
        SHAPE(vcaDistortion, 3.0, Analog),
        SHAPE(ladderStages, 3.0, Analog),
        SHAPE(otaLinearised, 4.0, Analog),
        SHAPE(bbdHeadroom, 1.5, Analog),
        SHAPE(speakerStiffness, 2.0, Analog),
        SHAPE(speakerBreakup, 3.0, Analog),
        SHAPE(rectifierSag, 3.0, Analog),
        SHAPE(singleEndedClassA, 2.0, Analog),
        SHAPE(pushPullAB, 2.0, Analog),
        SHAPE(cathodeFollower, 3.0, Analog),
        SHAPE(gridBlocking, 1.5, Analog),
        SHAPE(powerTubeDrive, 2.0, Analog),
        SHAPE(twoTriodeCascade, 3.0, Analog),
        SHAPE(vacuumDiode, 1.2, Analog),
        SHAPE(seleniumRectifier, 3.0, Analog),
        SHAPE(bridgeRectifier, 1.2, Analog),
        SHAPE(carbonMic, 3.0, Analog),
        SHAPE(springDriver, 2.0, Analog),
        SHAPE(inductorSat, 3.0, Analog),
        SHAPE(optoCell, 1.0, Analog),
        SHAPE(varMu, 4.0, Analog),
        SHAPE(fetLimiter, 3.0, Analog),
        SHAPE(diodeRingMod, 2.0, Folding),
        SHAPE(germaniumRingMod, 2.0, Folding),
        SHAPE(germaniumOctave, 6.0, Heavy),
        SHAPE(germaniumGated, 8.0, Analog),
        SHAPE(germaniumTempDrift, 8.0, Analog),
        SHAPE(diodeFeedbackLog, 2.0, Analog),
        SHAPE(zenerClamp, 1.5, Analog),
        SHAPE(neonBulbSnap, 1.2, Heavy),
        SHAPE(thyristorLatch, 1.0, Heavy),
        SHAPE(tapeBiasHiss, 2.0, Analog),
        SHAPE(wireRecorder, 2.0, Analog),

        // digital processes
        SHAPE(intOverflow8, 1.6, Digital),
        SHAPE(intOverflow16, 2.5, Digital),
        SHAPE(int4Overflow, 1.8, Digital),
        SHAPE(bitmaskAnd, 0.0, Digital),
        SHAPE(bitmaskXor, 0.0, Digital),
        SHAPE(bitShiftLeft, 0.0, Digital),
        SHAPE(grayCode, 0.0, Digital),
        SHAPE(bitReverse, 0.0, Digital),
        SHAPE(nibbleSwap, 0.0, Digital),
        SHAPE(msbStuck, 0.0, Digital),
        SHAPE(bitRotMask, 0.0, Digital),
        SHAPE(lsbFlip, 16.0, Digital),
        SHAPE(popCount, 0.0, Digital),
        SHAPE(lfsrScramble, 0.0, Digital),
        SHAPE(hashNoise, 0.4, Digital),
        SHAPE(muLawCrush, 6.0, Digital),
        SHAPE(aLawCompress, 0.0, Digital),
        SHAPE(truncQuant, 5.0, Digital),
        SHAPE(floorQuant, 5.0, Digital),
        SHAPE(midRiseQuant, 4.0, Digital),
        SHAPE(fractionalBits, 2.5, Digital),
        SHAPE(signMagnitudeError, 0.0, Digital),
        SHAPE(offsetBinaryGlitch, 0.0, Digital),
        SHAPE(float32Precision, 16.0, Digital),
        SHAPE(integerDivide, 1.0, Digital),
        SHAPE(pwmComparator, 2.0, Digital),
        SHAPE(moduloGain, 3.0, Digital),
        SHAPE(wrapCrush, 2.5, Digital),
        SHAPE(saturatingAdd8, 1.5, Digital),
        SHAPE(clipThenCrush, 2.0, Digital),
        SHAPE(crushThenTanh, 6.0, Digital),
        SHAPE(tableLookupNearest, 0.0, Digital),
        SHAPE(tableLookupLinear, 0.0, Digital),
        SHAPE(clippedAliasRing, 1.5, Digital),

        // fractal / recursive folding
        SHAPE(recursiveSineFold, 3.0, Folding),
        SHAPE(recursiveTriFold, 1.5, Folding),
        SHAPE(recursiveTanhSine, 2.0, Folding),
        SHAPE(mandelbrotOrbit, 12.0, Heavy),
        SHAPE(juliaFold, 3.0, Heavy),
        SHAPE(henonMap, 1.4, Heavy),
        SHAPE(cantorStaircase, 0.0, Digital),
        SHAPE(devilsFold, 3.0, Folding),
        SHAPE(weierstrassShape, 1.0, Folding),
        SHAPE(takagiBlancmange, 0.0, Folding),
        SHAPE(selfSimilarStairs, 3.0, Folding),
        SHAPE(tentMap, 5.0, Heavy),
        SHAPE(bakerMap, 4.0, Heavy),
        SHAPE(recursiveSaw, 2.0, Folding),
        SHAPE(feedbackPhaseFold, 4.0, Folding),
        SHAPE(negativeFeedbackTanh, 3.0, Analog),
        SHAPE(positiveFeedbackTanh, 2.0, Analog),
        SHAPE(nestedChebyshev3, 0.0, Folding),
        SHAPE(chebyshevChain, 0.0, Folding),
        SHAPE(repeatedRectify, 3.0, Folding),
        SHAPE(goldenFold, 2.0, Folding),
        SHAPE(dragonCurveBits, 0.0, Digital),
        SHAPE(thueMorse, 0.0, Digital),
        SHAPE(collatzSteps, 0.0, Heavy),
        SHAPE(fibonacciWord, 0.0, Digital),
        SHAPE(continuedFraction, 4.0, Analog),
        SHAPE(nestedAtan, 2.0, Analog),
        SHAPE(logSpiralFold, 6.0, Folding),
        SHAPE(fractalComb, 2.0, Folding),
        SHAPE(recursiveCrushFold, 1.5, Digital),
        SHAPE(duffingSpring, 3.0, Analog),
        SHAPE(recursiveRingSaw, 3.0, Folding),
        SHAPE(lSystemFold, 3.0, Folding),
        SHAPE(sierpinskiMask, 0.0, Digital),
        SHAPE(ifsBranchFold, 4.0, Folding),

        // inspired by minimal audio rift
        SHAPE(splitFold, 3.0, Folding),
        SHAPE(phaseWarp, 2.0, Folding),
        SHAPE(grind, 3.0, Digital),
        SHAPE(meltdown, 3.0, Folding),
        SHAPE(harmonicSwirl, 2.0, Folding),
        SHAPE(doubleFold, 2.0, Folding),
        SHAPE(smearClip, 2.0, Analog),
        SHAPE(stepFold, 4.0, Folding),
        SHAPE(resonant, 2.0, Folding),
        SHAPE(asymWrap, 3.0, Folding),
        SHAPE(crushFold, 3.0, Digital),
        SHAPE(octaveSplit, 3.0, Heavy),
        SHAPE(feedbackScream, 2.0, Heavy),
        SHAPE(rubberBand, 1.5, Folding),
        SHAPE(chaosFold, 6.0, Heavy),
        SHAPE(pulseWidth, 5.0, Heavy),
        SHAPE(slopeFold, 3.0, Folding),
        SHAPE(spectralTilt, 5.0, Folding),
        SHAPE(tubeFold, 3.0, Folding),
        SHAPE(glassShatter, 3.0, Folding),
        SHAPE(squashStretch, 0.0, Analog),
        SHAPE(bitFold, 3.0, Digital),
        SHAPE(envelopeFold, 4.0, Folding),
        SHAPE(sawFold, 4.0, Folding),
        SHAPE(combFold, 4.0, Folding),

        // inspired by output thermal
        SHAPE(heatCore, 4.0, Analog),
        SHAPE(glowTube, 3.0, Analog),
        SHAPE(meltRectify, 3.0, Analog),
        SHAPE(ember, 2.0, Analog),
        SHAPE(smolder, 3.0, Analog),
        SHAPE(blaze, 1.0, Heavy),
        SHAPE(scorch, 3.0, Folding),
        SHAPE(flashover, 2.0, Heavy),
        SHAPE(coolant, 3.0, Analog),
        SHAPE(convection, 6.0, Folding),
        SHAPE(radiant, 4.0, Analog),
        SHAPE(inferno, 2.0, Folding),
        SHAPE(plasma, 3.0, Heavy),
        SHAPE(furnaceBias, 3.0, Analog),
        SHAPE(heatSink, 3.0, Analog),
        SHAPE(thermistor, 3.0, Analog),
        SHAPE(boil, 1.0, Heavy),
        SHAPE(vapor, 3.0, Digital),
        SHAPE(fission, 3.0, Folding),
        SHAPE(exhaust, 2.0, Analog),
        SHAPE(charcoal, 10.0, Analog),
        SHAPE(magma, 5.0, Analog),
        SHAPE(spark, 0.5, Analog),
        SHAPE(afterglow, 3.0, Analog),
        SHAPE(runaway, 2.0, Heavy),

        // inspired by izotope trash 2
        SHAPE(scream, 1.0, Heavy),
        SHAPE(brokenSpeaker, 3.0, Analog),
        SHAPE(buzzBox, 3.0, Heavy),
        SHAPE(retroTransistor, 3.0, Analog),
        SHAPE(rectifyFull, 3.0, Analog),
        SHAPE(rectifyHalf, 3.0, Analog),
        SHAPE(gnarl, 1.0, Heavy),
        SHAPE(spikeFold, 4.0, Folding),
        SHAPE(mangle, 5.0, Digital),
        SHAPE(blownAmp, 2.0, Heavy),
        SHAPE(tapeEater, 1.0, Heavy),
        SHAPE(nuke, 1.0, Heavy),
        SHAPE(fryer, 1.0, Heavy),
        SHAPE(warmth, 3.0, Analog),
        SHAPE(megaFold, 1.0, Folding),
        SHAPE(sputter, 4.0, Digital),
        SHAPE(decimator, 2.0, Digital),
        SHAPE(crossRing, 2.0, Heavy),
        SHAPE(hollow, 3.0, Analog),
        SHAPE(fuzzBite, 8.0, Heavy),
        SHAPE(subRumble, 0.0, Analog),
        SHAPE(bitScream, 3.0, Digital),
        SHAPE(rattle, 1.0, Heavy),
        SHAPE(crunchStomp, 2.0, Analog),
        SHAPE(wreck, 2.0, Heavy),
        SHAPE(squeak, 1.0, Heavy),
    });

    #undef SHAPE

    // one group's shapes, smooth to bright. WaveshapeOrder.h holds the generated ones
    struct OrderEntry { int index; const char* name; };
    struct GroupOrder { const OrderEntry* entries; int count; };

    /*  A blend of up to four shapes: table indices, and weights that sum to 1. A position along a group's order
        (two neighbours) and a point on the XY map (the nearest few) both come down to one of these. */
    inline constexpr int maxMixShapes = 4;

    struct Mix {
        int count = 0;
        int index[maxMixShapes] = {};
        double weight[maxMixShapes] = {};
    };

    // a position along a group's order, 0 .. count - 1: the shape at or before it, and the next one weighted by how far on it is
    inline Mix mixAlong(const GroupOrder& order, double position) {
        position = std::clamp(position, 0.0, double(order.count - 1));
        int i = int(position);
        double frac = position - i;
        if (i >= order.count - 1) { i = order.count - 1; frac = 0.0; }

        Mix mix;
        mix.index[mix.count] = order.entries[i].index;
        mix.weight[mix.count++] = 1.0 - frac;
        if (frac > 0.0) {
            mix.index[mix.count] = order.entries[i + 1].index;
            mix.weight[mix.count++] = frac;
        }
        return mix;
    }

    // the blend's normalised curve. x is clamped to [-1, 1]
    inline double mixCurve(const Mix& mix, double x) {
        x = std::clamp(x, -1.0, 1.0);
        double y = 0.0;
        for (int i = 0; i < mix.count; i++)
            y += mix.weight[i] * shapes[(size_t) mix.index[i]](x);
        return y;
    }
    
    inline double morph(const GroupOrder& order, double position, double x) { return mixCurve(mixAlong(order, position), x); }

    inline constexpr double maxDriveDb = 36.0, driveStepDb = 0.25;
    inline constexpr int driveSteps = int(maxDriveDb / driveStepDb) + 1;
    inline constexpr double maxLoudnessGain = 1.5;

    // this is the db level the analysis signal was played at
    inline constexpr double analysisLevelDb = -3.0;

    // a blend's loudness gain at a drive: each shape's gain interpolated between drive steps, then weighted like the curves
    inline double mixLoudness(const Mix& mix, double driveDb, const double gains[][driveSteps]) {
        const double step = std::clamp(driveDb / driveStepDb, 0.0, double(driveSteps - 1));
        const int k = std::min(int(step), driveSteps - 2);
        const double t = step - k;

        double g = 0.0;
        for (int i = 0; i < mix.count; i++) {
            const double* row = gains[mix.index[i]];
            g += mix.weight[i] * (row[k] + (row[k + 1] - row[k]) * t);
        }
        return g;
    }

    inline double loudnessGain(const GroupOrder& order, double position, double driveDb, const double gains[][driveSteps]) {
        return mixLoudness(mixAlong(order, position), driveDb, gains);
    }

    /*  Blending curves that don't line up partly cancels them out: unrelated curves at even weights come out
        quieter than any one of them, and away from the dots the nearest few are both less alike and more evenly
        weighted */
    inline constexpr double maxRelevel = 2.0;

    inline double mixRelevel(const Mix& mix, double driveDb, const double gains[][driveSteps]) {
        if (mix.count < 2) return 1.0;

        constexpr int N = 128;
        const double amplitude = std::pow(10.0, (analysisLevelDb + driveDb) / 20.0);
        double blend[N] = {}, expected = 0.0;

        for (int i = 0; i < mix.count; i++) {
            const Mix one { 1, { mix.index[i] }, { 1.0 } };
            const double gain = mixLoudness(one, driveDb, gains);

            double shaped[N], mean = 0.0;
            for (int n = 0; n < N; n++) {
                shaped[n] = shapes[(size_t) mix.index[i]](std::tanh(amplitude * std::sin(2.0 * M_PI * n / N))) * gain;
                mean += shaped[n] / N;
            }

            double power = 0.0;
            for (int n = 0; n < N; n++) {
                power += (shaped[n] - mean) * (shaped[n] - mean);
                blend[n] += mix.weight[i] * shaped[n];
            }
            expected += mix.weight[i] * std::sqrt(power / N);
        }

        double mean = 0.0, power = 0.0;
        for (double v : blend) mean += v / N;
        for (double v : blend) power += (v - mean) * (v - mean);
        const double level = std::sqrt(power / N);
        return level * maxRelevel > expected ? std::max(expected / level, 1.0) : maxRelevel;
    }

    struct MapPoint { int index; const char* name; double x, y; };

    inline constexpr int mapNeighbours = maxMixShapes;

    // How strongly the nearest shape wins
    inline constexpr double mapSharpness = 2.0;

    inline Mix mixAt(const MapPoint* points, int count, double x, double y) {
        // the mapNeighbours + 1 nearest, closest first. the last only sets R
        int nearest[mapNeighbours + 1] = {};
        double distance[mapNeighbours + 1] = {};
        int found = 0;
        for (int p = 0; p < count; p++) {
            const double d = std::hypot(points[p].x - x, points[p].y - y);
            if (found == mapNeighbours + 1 && d >= distance[mapNeighbours]) continue;

            int slot = found < mapNeighbours + 1 ? found++ : mapNeighbours;
            while (slot > 0 && distance[slot - 1] > d) {
                distance[slot] = distance[slot - 1];
                nearest[slot] = nearest[slot - 1];
                slot--;
            }
            distance[slot] = d;
            nearest[slot] = p;
        }

        Mix mix;
        auto only = [&](int p) { mix.count = 1; mix.index[0] = points[p].index; mix.weight[0] = 1.0; return mix; };
        if (found == 0) return mix;
        if (distance[0] <= 0.0) return only(nearest[0]);

        const int used = std::min(found, mapNeighbours);
        const double R = found > mapNeighbours ? distance[mapNeighbours] : distance[found - 1] * (1.0 + 1e-9);

        double total = 0.0;
        for (int i = 0; i < used; i++) {
            const double w = std::pow((R - distance[i]) / (R * distance[i]), mapSharpness);
            mix.index[mix.count] = points[nearest[i]].index;
            mix.weight[mix.count++] = w;
            total += w;
        }
        if (total <= 0.0) return only(nearest[0]); // every one of them as far as the next: nothing to prefer

        for (int i = 0; i < mix.count; i++) mix.weight[i] /= total;
        return mix;
    }

    inline constexpr double maxAsymK = 10.0;

    inline double skew(double v, double asym) {
        const double k = asym * maxAsymK;
        return k > 1e-6 ? std::expm1(std::min(k * v, 20.0)) / k : v;
    }

    // the bias knob, -1 .. 1, squared with its sign kept, so the first part of the knob only nudges the curve
    inline double biasCurve(double bias) { return bias * std::fabs(bias); }

    // the smooth knob, 0 .. 1, to how far either side of a point the curve gets averaged. squared so the low end is finer
    inline constexpr double maxSmoothWidth = 0.5;

    inline double smoothToWidth(double smooth) { return maxSmoothWidth * smooth * smooth; }

    struct CurveTable {
        static constexpr int cells = 8192;
        static constexpr double span = 1.0 + maxSmoothWidth; // the table covers [-span, span]
        static constexpr double cell = 2.0 * span / cells;

        CurveTable() : values(cells + 1), scratch(cells + 1), filled(cells + 1, 0) {}

        void build(const Mix& mix, double width) {
            const double box = 0.5 * width / cell; // in cells
            lazy = box <= 1e-9;
            if (lazy) {
                lazyMix = mix;
                ++generation; // every cell stale. ponytail: wraps after 4 billion rebuilds
                return;
            }

            const int first = int(std::ceil((span - 1.0) / cell)), last = cells - first; // the points inside [-1, 1]
            for (int i = first; i <= last; i++)
                values[(size_t) i] = mixCurve(mix, -span + i * cell);
            std::fill(values.begin(), values.begin() + first, mixCurve(mix, -1.0));
            std::fill(values.begin() + last + 1, values.end(), mixCurve(mix, 1.0));

            for (int pass = 0; pass < 4; pass++)
                boxFilter(box);
        }

        void build(const GroupOrder& order, double position, double width) { build(mixAlong(order, position), width); }

        double read(double x) const {
            const double pos = (std::clamp(x, -1.0, 1.0) + span) / cell;
            const int i = std::min(int(pos), cells - 1);
            if (lazy) {
                fill(i);
                fill(i + 1);
            }
            return values[(size_t) i] + (values[(size_t) i + 1] - values[(size_t) i]) * (pos - i);
        }

    private:
        // mixCurve clamps x, so the cells past [-1, 1] come out flat like build() makes them
        void fill(int i) const {
            if (filled[(size_t) i] == generation) return;
            values[(size_t) i] = mixCurve(lazyMix, -span + i * cell);
            filled[(size_t) i] = generation;
        }

        // the tent a point's value spreads over, max(0, 1 - |u|), integrated from -infinity up to u
        static double tentIntegral(double u) {
            if (u <= -1.0) return 0.0;
            if (u <= 0.0) return 0.5 * (u + 1.0) * (u + 1.0);
            if (u < 1.0) return 1.0 - 0.5 * (1.0 - u) * (1.0 - u);
            return 1.0;
        }

        void boxFilter(double box) {
            const double half = 0.5 * box;
            const int inner = int(std::floor(half - 1.0)); // points out to here weigh exactly 1 / box, -1 when there are none
            const int outer = int(std::ceil(half + 1.0)) - 1; // past here they weigh nothing

            const int firstEdge = std::max(inner + 1, 0);
            double edgeWeight[2] = {};
            for (int j = firstEdge; j <= outer; j++)
                edgeWeight[j - firstEdge] = (tentIntegral(half - j) - tentIntegral(-half - j)) / box;

            auto at = [&](int j) { return values[(size_t) std::clamp(j, 0, cells)]; };

            double inside = 0.0;
            for (int j = -inner; j <= inner; j++) inside += at(j);

            for (int i = 0; i <= cells; i++) {
                double y = inside / box;
                for (int j = firstEdge; j <= outer; j++)
                    y += edgeWeight[j - firstEdge] * (j == 0 ? at(i) : at(i - j) + at(i + j));
                scratch[(size_t) i] = y;

                if (inner >= 0)
                    inside += at(i + inner + 1) - at(i - inner);
            }
            values.swap(scratch);
        }

        // values is filled in by read() when lazy
        mutable std::vector<double> values;
        std::vector<double> scratch;
        mutable std::vector<uint32_t> filled; // the generation each cell was last filled in
        uint32_t generation = 0;
        bool lazy = false;
        Mix lazyMix;
    };
}
