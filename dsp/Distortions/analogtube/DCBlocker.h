
#pragma once


class DCBlocker {
public:
    DCBlocker();
    ~DCBlocker();

    float processSample(float x) {
        float y = x - xm1 + 0.995 * ym1;
        xm1 = x;
        ym1 = y;
        return y;
    }

private:

    float xm1 = 0.f;
    float ym1 = 0.f;
};