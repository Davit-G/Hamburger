#pragma once

#include <cstddef>
#include <cstdint>

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

    static constexpr size_t triggerDecimation = 8; // the trigger is searched for on a decimated copy, so it's way cheaper
    static constexpr size_t triggerMatchLength = 32; // how much of the waveform is matched against the previous frame
    
    static constexpr std::uint8_t contextLabelAlpha = 20; // brightness of the watermark behind each view
    static constexpr float readoutFontHeight = 11.0f; // the readouts along the top of each view
}