class HilbertTransform
{
public:
    HilbertTransform() :
        z1_1(0), z1_2(0), z1_3(0), z1_4(0),
        z2_1(0), z2_2(0), z2_3(0),
        a1_1(0.6923878), a1_2(0.9360654322959),
        a1_3(0.9882295226860), a1_4(0.9987488452737),
        a2_1(0.4021921162426), a2_2(0.8561710882420),
        a2_3(0.9722909545651), a2_4(0.9952884791278)
    {}

    float getI(){return I;}
    float getQ(){return Q;}

    void processSample(float x)
    {
        float y1 = a1_1 * (x - z1_1) + z1_2;
        float y2 = a1_2 * (y1 - z1_2) + z1_3;
        float y3 = a1_3 * (y2 - z1_3) + z1_4;
        float y4 = a1_4 * (y3 - z1_4);
        z1_1 = x;
        z1_2 = y1;
        z1_3 = y2;
        z1_4 = y3;

        float w1 = a2_1 * (x - z2_1) + z2_2;
        float w2 = a2_2 * (w1 - z2_2) + z2_3;
        float w3 = a2_3 * (w2 - z2_3);
        z2_1 = x;
        z2_2 = w1;
        z2_3 = w2;

        I = 0.5 * (y4 + w3);
        Q = 0.5 * (y4 - w3);
    }

private:
    float z1_1, z1_2, z1_3, z1_4;
    float z2_1, z2_2, z2_3;
    const float a1_1, a1_2, a1_3, a1_4;
    const float a2_1, a2_2, a2_3, a2_4;
    

    float I;
    float Q;
};
