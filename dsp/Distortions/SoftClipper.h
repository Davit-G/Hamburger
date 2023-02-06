#include <cmath>

float softClipSample(float input)
{
    // soft clipper using atan

    return std::atan(input) / (3.1415 / 2);
}
