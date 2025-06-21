#include "HysteresisProcessing.h"
#include <cmath>

HysteresisProcessing::HysteresisProcessing() = default;

void HysteresisProcessing::reset()
{
    M_n1 = 0.0;
    H_n1 = 0.0;
    H_d_n1 = 0.0;

    hpState.coth = 0.0;
    hpState.nearZero = false;
}

void HysteresisProcessing::setSampleRate (double newSR)
{
    fs = newSR;
    T = 1.0 / fs;
    Talpha = T / 1.9;
}

void HysteresisProcessing::cook (double drive, double width, double sat)
{
    hpState.M_s = 0.5 + 1.5 * (1.0 - sat);
    hpState.a = hpState.M_s / (0.01 + 6.0 * drive);
    hpState.c = std::sqrt (1.0f - width) - 0.01;
    hpState.k = 0.47875;
    upperLim = 20.0;

    hpState.nc = 1.0 - hpState.c;
    hpState.M_s_oa = hpState.M_s / hpState.a;
    hpState.M_s_oa_talpha = hpState.alpha * hpState.M_s_oa;
    hpState.M_s_oa_tc = hpState.c * hpState.M_s_oa;
    hpState.M_s_oa_tc_talpha = hpState.alpha * hpState.M_s_oa_tc;
    hpState.M_s_oaSq_tc_talpha = hpState.M_s_oa_tc_talpha / hpState.a;
    hpState.M_s_oaSq_tc_talphaSq = hpState.alpha * hpState.M_s_oaSq_tc_talpha;
}
