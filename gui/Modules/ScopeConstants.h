#pragma once

namespace scope_constants
{
    static constexpr int defaultQueueSize = 2048;
    static constexpr int fftSize = 2048;
    static constexpr int fftInputSize = fftSize * 2;
    static constexpr int fftBins = fftSize / 2 + 1;
    static constexpr int defaultFrameRate = 60;
    static constexpr int defaultHopSize = 44100 / defaultFrameRate;
    static constexpr float spectrumSmoothing = 0.6f;

    // the frequency range the x axis spans, shared by every curve drawn over the spectrum
    static constexpr double minDrawFreq = 20.0;
    static constexpr double maxDrawFreq = 20000.0;
}