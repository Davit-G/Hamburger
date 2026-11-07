/*  Harmonic features of every shape, for mapping them in 2D with UMAP (umap_shapes.py).

    Each shape is played the way the plugin plays it -- a sine at analysisLevelDb, driven, through tanh, the
    curve, then that drive's loudness gain -- at a few drives, and harmonicAnalyse() takes the first 512
    harmonics of each. The loudness gain keeps level out of it, so shapes end up near each other for sounding
    alike rather than for being about as loud.

    One CSV row per shape: name, group, table index, then per drive its spectral flatness, oddness and
    brightness (power weighted mean harmonic number, 1 for a pure sine), then every harmonic in dB per drive.

      harmonic_features [out.csv]      default build/harmonic_features.csv. takes a couple of minutes */

#define main features_main
#include "../features.cpp"
#undef main

#ifndef HAVE_ORDER_HEADER
#error "generate WaveshapeOrder.h first: waveshape_features --header > dsp/Distortions/waveshape/WaveshapeOrder.h"
#endif

#include <fstream>
#include <sstream>

int main(int argc, char** argv) {
    const char* path = argc > 1 ? argv[1] : "build/harmonic_features.csv";
    const double drives[] = { 0.0, 12.0, 24.0, 36.0 };
    const int totalSamples = (int)sampleRate * seconds, harmonics = 512;

    std::ofstream out(path);
    if (!out) {
        fprintf(stderr, "can't write %s\n", path);
        return 1;
    }

    out << "name,group,index";
    for (double driveDb : drives)
        out << ",flatness_" << (int)driveDb << ",oddness_" << (int)driveDb << ",brightness_" << (int)driveDb;
    for (double driveDb : drives)
        for (int h = 1; h <= harmonics; h++)
            out << ",h" << h << "_" << (int)driveDb;
    out << "\n";

    std::vector<double> signal(totalSamples), magnitudes(harmonics);

    for (size_t s = 0; s < shapes.size(); s++) {
        const OrderEntry entry { (int)s, shapes[s].name };
        const GroupOrder one { &entry, 1 };
        std::ostringstream descriptors, spectrum;

        for (double driveDb : drives) {
            const double amplitude = std::pow(10.0, (analysisLevelDb + driveDb) / 20.0);
            const double gain = loudnessGain(one, 0.0, driveDb, loudnessGains);
            for (int i = 0; i < totalSamples; i++)
                signal[i] = shapes[s](std::tanh(amplitude * sinewave((float)i, baseFreq))) * gain;

            const HarmonicResults result = harmonicAnalyse(signal, magnitudes.data(), totalSamples, harmonics);

            double power = 0.0, weighted = 0.0;
            for (int h = 0; h < harmonics; h++) {
                power += magnitudes[h] * magnitudes[h];
                weighted += (h + 1) * magnitudes[h] * magnitudes[h];
            }

            descriptors << "," << result.spectralFlatness << "," << result.oddness << "," << (power > 0.0 ? weighted / power : 0.0);
            for (int h = 0; h < harmonics; h++)
                spectrum << "," << 20.0 * std::log10(magnitudes[h] + 1e-12);
        }

        out << shapes[s].name << "," << groupNames[shapes[s].group] << "," << s << descriptors.str() << spectrum.str() << "\n";
        fprintf(stderr, "\r%zu / %zu shapes", s + 1, shapes.size());
    }

    fprintf(stderr, "\nwrote %s\n", path);
}
