#ifndef HYSTERESISPROCESSING_H_INCLUDED
#define HYSTERESISPROCESSING_H_INCLUDED

#include <cmath>

#include "HysteresisOps.h"

/*
    Hysteresis processing for a model of an analog tape machine.
    For more information on the DSP happening here, see:
    https://ccrma.stanford.edu/~jatin/420/tape/TapeModel_DAFx.pdf
*/
class HysteresisProcessing
{
public:
    HysteresisProcessing();
    HysteresisProcessing (HysteresisProcessing&&) noexcept = default;

    void reset();
    void setSampleRate (double newSR);

    void cook (double drive, double width, double sat);

    /* Process a single sample */
    template <typename Float>
    inline Float process (Float H) noexcept
    {
        auto H_d = HysteresisOps::deriv (H, H_n1, H_d_n1, (Float) T);

        Float M = NRSolver<4> (H, H_d);

        // check for instability
#if HYSTERESIS_USE_SIMD
        auto notIllCondition = ! (xsimd::isnan (M) || (M > upperLim));
        M = xsimd::select (notIllCondition, M, (Float) 0.0);
        H_d = xsimd::select (notIllCondition, H_d, (Float) 0.0);
#else
        bool illCondition = std::isnan (M) || M > upperLim;
        M = illCondition ? 0.0 : M;
        H_d = illCondition ? 0.0 : H_d;
#endif
        M_n1 = M;
        H_n1 = H;
        H_d_n1 = H_d;

        return M;
    }

private:
    // newton-raphson solvers
    template <int nIterations, typename Float>
    inline Float NRSolver (Float H, Float H_d) noexcept
    {
        Float M = M_n1;
        const Float last_dMdt = HysteresisOps::hysteresisFunc (M_n1, H_n1, H_d_n1, hpState);

        Float dMdt;
        Float dMdtPrime;
        Float deltaNR;
        for (int n = 0; n < nIterations; ++n)
        {
            using namespace HysteresisOps;
            dMdt = hysteresisFunc (M, H, H_d, hpState);
            dMdtPrime = hysteresisFuncPrime (H_d, dMdt, hpState);
            deltaNR = (M - M_n1 - (Float) Talpha * (dMdt + last_dMdt)) / (Float (1.0) - (Float) Talpha * dMdtPrime);
            M -= deltaNR;
        }

        return M;
    }

    // parameter values
    double fs = 48000.0;
    double T = 1.0 / fs;
    double Talpha = T / 1.9;
    double upperLim = 20.0;

    // state variables
#if HYSTERESIS_USE_SIMD
    xsimd::batch<double> M_n1 = 0.0;
    xsimd::batch<double> H_n1 = 0.0;
    xsimd::batch<double> H_d_n1 = 0.0;
#else
    double M_n1 = 0.0;
    double H_n1 = 0.0;
    double H_d_n1 = 0.0;
#endif

    HysteresisOps::HysteresisState hpState;
};

#endif
