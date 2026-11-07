// cmake --build build --target example_dsplib && clang++ -std=c++17 dsptests/hotdsp.cpp -o /tmp/hotdsp && /tmp/hotdsp build/hotdsp/example_dsplib.dylib
// loads the hot dsp the way the DLL distortion does, twice from two copies like a reload, and runs a block through each
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>
#include <memory>
#include <string>

#include "../hotdsp/HotDsp.h"

int main (int argc, char** argv)
{
    assert (argc == 2);
    void* handles[2] {};

    for (int copy = 0; copy < 2; ++copy)
    {
        const auto path = "/tmp/hotdsp_test_" + std::to_string (copy) + ".dylib";
        std::system (("cp '" + std::string (argv[1]) + "' " + path).c_str());

        handles[copy] = dlopen (path.c_str(), RTLD_LOCAL | RTLD_NOW);
        assert (handles[copy] != nullptr);

        auto create = (HotDspCreate) dlsym (handles[copy], "hotdspCreate");
        assert (create != nullptr);

        const HotDspParam infos[] { { "Drive", -24, 24, 0 }, { "A", 0, 1, 0.5f } };
        std::unique_ptr<HotDsp> dsp (create());
        dsp->prepare ({ 48000.0, 2, 64, infos, 2 });

        float l[64], r[64], drive[64], a[64];
        float* io[] { l, r };
        const float* params[] { drive, a };

        for (int n = 0; n < 64; ++n)
        {
            l[n] = r[n] = 2.0f * std::sin (0.1f * n);
            drive[n] = 12.0f;
            a[n] = 0.5f;
        }

        dsp->process (io, 2, 40, params, 2); // shorter than prepared, like some hosts

        for (int n = 0; n < 40; ++n)
            assert (std::isfinite (l[n]) && std::abs (l[n]) <= 1.0f);

        assert (l[39] != 2.0f * std::sin (0.1f * 39)); // it did something
        assert (r[40] == 2.0f * std::sin (0.1f * 40));  // and nothing past numSamples
    }

    assert (handles[0] != handles[1]); // two copies are two libraries, which is what makes a reload load the new code
    std::puts ("ok");
}
