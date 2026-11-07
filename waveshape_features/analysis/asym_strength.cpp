/*  What full asym does at different maximum k (maxAsymK in Waveshapes.h), on the first analog curve with a sine
    at analysisLevelDb and bias at 0: the even harmonics it adds, how different the output is from asym 0, and
    the level change. A skew that fades out as drive rises has mostly turned into DC, which the plugin's
    highpass takes away.

      asym_strength [k ...]      default: maxAsymK first, then 3 6 10 20 */

#define main features_main
#include "../features.cpp"
#undef main

#ifndef HAVE_ORDER_HEADER
#error "generate WaveshapeOrder.h first: waveshape_features --header > dsp/Distortions/waveshape/WaveshapeOrder.h"
#endif

const int periodSamples = 4096, sineBin = 16; // a whole number of periods, so each harmonic lands on one bin

double harmonicMagnitude(const std::vector<double>& y, int bin) {
    double re = 0.0, im = 0.0;
    for (int n = 0; n < periodSamples; n++) {
        re += y[n] * std::cos(2.0 * M_PI * bin * n / periodSamples);
        im += y[n] * std::sin(2.0 * M_PI * bin * n / periodSamples);
    }
    return 2.0 * std::sqrt(re * re + im * im) / periodSamples;
}

int main(int argc, char** argv) {
    std::vector<double> ks;
    for (int i = 1; i < argc; i++) ks.push_back(std::atof(argv[i]));
    if (ks.empty()) {
        ks = { maxAsymK };
        for (double k : { 3.0, 6.0, 10.0, 20.0 })
            if (k != maxAsymK) ks.push_back(k);
    }

    const GroupOrder& analog = groupOrders[Analog];
    printf("full asym on %s: even harmonics / change vs asym 0 / level change\n", shapes[analog.entries[0].index].name);
    printf("%-8s", "max k");
    for (double driveDb : { 0.0, 12.0, 24.0, 36.0 }) printf("   %28s", ("drive " + std::to_string((int)driveDb)).c_str());
    printf("\n");

    for (double k : ks) {
        printf("%-8g", k);
        for (double driveDb : { 0.0, 12.0, 24.0, 36.0 }) {
            const double amplitude = std::pow(10.0, (analysisLevelDb + driveDb) / 20.0);
            const double asym = k / maxAsymK; // skew() scales k by the knob, so this lands on k
            std::vector<double> plain(periodSamples), skewed(periodSamples);
            for (int n = 0; n < periodSamples; n++) {
                double x = amplitude * std::sin(2.0 * M_PI * sineBin * n / periodSamples);
                plain[n] = morph(analog, 0, std::tanh(x)) - morph(analog, 0, 0.0);
                skewed[n] = morph(analog, 0, std::tanh(skew(x, asym))) - morph(analog, 0, std::tanh(skew(0.0, asym)));
            }

            double fundamental = harmonicMagnitude(skewed, sineBin), even = 0.0;
            for (int h = 2; h <= 20; h += 2) even += std::pow(harmonicMagnitude(skewed, sineBin * h), 2);

            // DC left out of both, as the highpass would
            double meanPlain = 0.0, meanSkewed = 0.0, difference = 0.0, plainEnergy = 0.0, skewedEnergy = 0.0;
            for (int n = 0; n < periodSamples; n++) { meanPlain += plain[n] / periodSamples; meanSkewed += skewed[n] / periodSamples; }
            for (int n = 0; n < periodSamples; n++) {
                double a = plain[n] - meanPlain, b = skewed[n] - meanSkewed;
                difference += (b - a) * (b - a);
                plainEnergy += a * a;
                skewedEnergy += b * b;
            }
            printf("   %6.1f / %6.1f / %+5.1f dB", 10.0 * std::log10(even / (fundamental * fundamental)),
                   10.0 * std::log10(difference / plainEnergy), 10.0 * std::log10(skewedEnergy / plainEnergy));
        }
        printf("\n");
    }
}
