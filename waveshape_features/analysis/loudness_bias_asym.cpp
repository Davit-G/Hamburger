/*  How far the bias and asym knobs pull loudness off what the table was built for (both at 0). Runs the
    plugin's per sample path -- drive, bias curve, asym skew, tanh, the curve minus where the bias alone lands,
    loudness gain, DC out -- with a sine at analysisLevelDb, over every shape in playing order.

      jump:     level change between neighbouring shapes, every group: 90th percentile / worst
      vs table: how far each shape's level moved from its bias 0 / asym 0 level, median */

#define main features_main
#include "../features.cpp"
#undef main

#ifndef HAVE_ORDER_HEADER
#error "generate WaveshapeOrder.h first: waveshape_features --header > dsp/Distortions/waveshape/WaveshapeOrder.h"
#endif

double pathLevel(const GroupOrder& order, int i, double driveDb, double biasKnob, double asymKnob) {
    const int N = 4096;
    const double amplitude = std::pow(10.0, (analysisLevelDb + driveDb) / 20.0);
    const double offset = biasCurve(biasKnob);
    const double rest = morph(order, i, std::tanh(skew(offset, asymKnob)));
    const double gain = loudnessGain(order, i, driveDb, loudnessGains);

    double sum = 0.0, sumSquares = 0.0;
    for (int n = 0; n < N; n++) {
        double y = (morph(order, i, std::tanh(skew(amplitude * std::sin(2.0 * M_PI * n / N) + offset, asymKnob))) - rest) * gain;
        sum += y;
        sumSquares += y * y;
    }
    double mean = sum / N;
    return std::sqrt(std::max(sumSquares / N - mean * mean, 1e-30));
}

int main() {
    const double drives[] = { 0.0, 18.0, 36.0 };

    // every shape's level at bias 0 / asym 0, per drive
    std::vector<std::vector<double>> base(std::size(drives));
    for (size_t d = 0; d < std::size(drives); d++)
        for (int g = 0; g < GroupCount; g++)
            for (int i = 0; i < groupOrders[g].count; i++)
                base[d].push_back(pathLevel(groupOrders[g], i, drives[d], 0.0, 0.0));

    printf("%-20s", "bias / asym knob");
    for (double driveDb : drives) printf("   drive %2.0f: jump 90%%/worst, vs table", driveDb);
    printf("\n");

    const std::pair<double, double> knobs[] = { {0.0, 0.0}, {0.3, 0.0}, {0.7, 0.0}, {1.0, 0.0}, {0.0, 0.3}, {0.0, 0.7}, {0.0, 1.0}, {0.7, 1.0} };
    for (auto [biasKnob, asymKnob] : knobs) {
        printf("bias %.1f asym %.1f    ", biasKnob, asymKnob);
        for (size_t d = 0; d < std::size(drives); d++) {
            std::vector<double> jumps, shifts;
            size_t shape = 0;
            for (int g = 0; g < GroupCount; g++) {
                double previous = 0.0;
                for (int i = 0; i < groupOrders[g].count; i++, shape++) {
                    double level = pathLevel(groupOrders[g], i, drives[d], biasKnob, asymKnob);
                    if (i) jumps.push_back(std::fabs(20.0 * std::log10(level / previous)));
                    shifts.push_back(std::fabs(20.0 * std::log10(level / base[d][shape])));
                    previous = level;
                }
            }
            std::sort(jumps.begin(), jumps.end());
            std::sort(shifts.begin(), shifts.end());
            printf("   %14.1f/%5.1f dB, %5.1f dB", jumps[jumps.size() * 9 / 10], jumps.back(), shifts[shifts.size() / 2]);
        }
        printf("\n");
    }
}
