/*  How closely the plugin's CurveTable follows the exact curves, shape by shape. Worth a run after adding
    shapes, jumpy ones especially: the table draws every jump as a ramp one cell wide, so bit tricks and
    wraps land far below the smooth curves.

      smooth 0%:  table vs the exact morph. the plugin plays the morph itself at smooth 0, so this is what
                  moving the knob off 0 changes on each shape
      smooth > 0: table vs the exact curve smoothed by the same cubic B-spline, as a 1601 point convolution

    Error is RMS relative to the reference's own RMS, so a shape that smooths down to almost nothing reads
    worse here than it sounds. */

#define main features_main
#include "../features.cpp"
#undef main

#ifndef HAVE_ORDER_HEADER
#error "generate WaveshapeOrder.h first: waveshape_features --header > dsp/Distortions/waveshape/WaveshapeOrder.h"
#endif

#include <random>

int main() {
    static CurveTable table; // two 65k point buffers, off the stack
    const int taps = 1601, inputs = 300;

    for (double knob : { 0.0, 0.05, 0.2, 0.5 }) {
        const double width = smoothToWidth(knob);

        // cubic B-spline over [-width, width], midpoint rule
        std::vector<double> offset(taps), weight(taps);
        double total = 0.0;
        for (int k = 0; k < taps; k++) {
            double t = -2.0 + 4.0 * (k + 0.5) / taps, a = std::fabs(t);
            offset[k] = 0.5 * t * width;
            weight[k] = a < 1.0 ? 2.0 / 3.0 - a * a + 0.5 * a * a * a : (2.0 - a) * (2.0 - a) * (2.0 - a) / 6.0;
            total += weight[k];
        }
        for (double& w : weight) w /= total;

        struct Row { const char* name; double errorDb; };
        std::vector<Row> rows;
        std::mt19937 rng(3);
        std::uniform_real_distribution<double> uniform(-1.0, 1.0);

        for (size_t s = 0; s < shapes.size(); s++) {
            const OrderEntry entry { (int)s, shapes[s].name };
            const GroupOrder one { &entry, 1 };
            table.build(one, 0.0, width);

            double error = 0.0, signal = 0.0;
            for (int n = 0; n < inputs; n++) {
                double x = uniform(rng), reference = 0.0;
                if (width > 0.0)
                    for (int k = 0; k < taps; k++) reference += weight[k] * morph(one, 0.0, x + offset[k]);
                else
                    reference = morph(one, 0.0, x);
                double e = table.read(x) - reference;
                error += e * e;
                signal += reference * reference;
            }
            rows.push_back({ shapes[s].name, 10.0 * std::log10(error / std::max(signal, 1e-30) + 1e-30) });
        }

        std::sort(rows.begin(), rows.end(), [](auto& a, auto& b) { return a.errorDb > b.errorDb; });
        int over60 = 0, over40 = 0;
        for (auto& row : rows) { over60 += row.errorDb > -60.0; over40 += row.errorDb > -40.0; }
        printf("smooth %3.0f%%: median %5.0f dB, %3d shapes worse than -60 dB, %3d worse than -40 dB. worst:",
               knob * 100.0, rows[rows.size() / 2].errorDb, over60, over40);
        for (int i = 0; i < 6; i++) printf(" %s %.0f,", rows[i].name, rows[i].errorDb);
        printf("\n");
    }
}
