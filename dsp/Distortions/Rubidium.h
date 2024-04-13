#pragma once

#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"

/*

@init
// smooth fader
shape = 4;
dt = 1/srate;
adj_s3 = 1;
adj_s4 = 1;
// nested allpass
cDenorm = 10^-30 ;
v_n_buf0 = 1000;
v_n_buf1 = 2000;
v_n_buf2 = 3000;
v_n_buf3 = 4000;
v_n_buf4 = 5000;
v_n_buf5 = 6000;
g = 0;
// highpass
h0 = 0;
h1 = 0;
h2 = 0;
h3 = 0;
// lowpass
l0 = 0;
l1 = 0;
l2 = 0;
l3 = 0;
// filter randomizer
rand0 = 0.05 * (rand(2)-1);
rand1 = 0.05 * (rand(2)-1);
// single pole allpass
ap1_k0 = -0.9;
ap1_k1 = -0.9;

@slider
gain = 0.125+(slider1/15);
hys0 = 0.015625+0.015625*(slider2/12.5);
hys1 = slider2/100;
cut0  = 2 * 3.1415926585f * (highpassFreq+rand0)/srate;
cut1  = 2 * 3.1415926585f * (highpassFreq+rand1)/srate;
cut2 = 2 * 3.1415926585f * 0.01/srate;
buf_length1 = max((slider4*2)-1,0);
slider4 == 6 ? buf_length1 = 21;
buf_length0 = max(buf_length1*buf_length1*2,1);
buf_length1 *= 2;
buf_length1 == 2 ? buf_length0 = 6;
mode = slider5;
vol = 10^(slider6/20);
// smooth fader
adj3 = gain <= 0 ? 0 : exp(shape * log10(gain));
adj4 = gain <= 0 ? 0 : exp(shape * log10(gain));
rc = 0.2 * max(128 * 0.001, sampleblock / srate);
a = dt / (rc + dt);

@sample


spl0 = atan(spl0 * 1.6);
spl1 = atan(spl1 * 1.6);

// smooth fader gain
adj_s3 += a * (adj3 - adj_s3);
!adj3 && !adj_s3 ? adj_s3 = adj3;
adj_s4 += a * (adj4 - adj_s4);
!adj4 && !adj_s4 ? adj_s4 = adj4;
spl0 *= adj_s3;
spl1 *= adj_s4;

// delta signal hysteresis - single pole allpass
ap1_x0 = spl0;
ap1_y0 = ap1_x0 - ap1_k0 * ( ap1_y = ap1_y0 + ap1_k0 * ap1_x0 );
delta0 = ap1_y;
ap1_x1 = spl1;
ap1_y1 = ap1_x1 - ap1_k1 * ( ap1_z = ap1_y1 + ap1_k1 * ap1_x1 );
delta1 = ap1_z;

// low level hysteresis - nested allpass
in0 = ap1_y + cDenorm;;
v_n0 = in0 + v_n_buf0[pos0] * -g;
out0 = v_n0 * g + v_n_buf0[pos0];
v_n_buf0[pos0] = v_n0;
pos0 += 1;
pos0 > buf_length0 ? pos0 = 0;
in1 = ap1_z + cDenorm;;
v_n1 = in1 + v_n_buf1[pos1] * -g;
out1 = v_n1 * g + v_n_buf1[pos1];
v_n_buf1[pos1] = v_n1;
pos1 += 1;
pos1 > buf_length0 ? pos1 = 0;
delta0 += hys1*max(hys0-sqrt(out0),0);
delta1 += hys1*max(hys0-sqrt(out1),0);

// delta lowpass + rectified
delta0 = sqr(l0+=((delta0-l0)*cut0));
delta1 = sqr(l1+=((delta1-l1)*cut1));

// signal highpass
spl0 -= (h0+=((spl0-h0)*cut0));
spl1 -= (h1+=((spl1-h1)*cut1));

// saturation
spl0 = atan(spl0 * delta0)  / (spl0 === 0 ? 1 : delta0);
spl1 = atan(spl1 * delta1)  / (spl1 === 0 ? 1 : delta1) ;
//spl0 = sin(spl0 * delta0) / (spl0 === 0 ? 1 : delta0);
//spl1 = sin(spl1 * delta1) / (spl1 === 0 ? 1 : delta1);
// phase rotation impedance- / step ratio - nested allpass

buf_length1 == 0 ?:
(

// signal
in2 = spl0 + cDenorm;
v_n2 = in2 + v_n_buf2[pos2] * -g;
out2 = v_n2 * g + v_n_buf2[pos2];
v_n_buf2[pos2] = v_n2;
pos2 += 1;
pos2 > buf_length1 ? pos2 = 0;
spl0 = out2;
in3 = spl1 + cDenorm;
v_n3 = in3 + v_n_buf3[pos3] * -g;
out3 = v_n3 * g + v_n_buf3[pos3];
v_n_buf3[pos3] = v_n3;
pos3 += 1;
pos3 > buf_length1 ? pos3 = 0;
spl1 = out3;

// delta
in4 = delta0 + cDenorm;
v_n4 = in4 + v_n_buf4[pos4] * -g;
delta0 = v_n4 * g + v_n_buf4[pos4];
v_n_buf4[pos4] = v_n4;
pos4 += 1;
pos4 > buf_length1 ? pos4 = 0;
in5 = delta1 + cDenorm;
v_n5 = in5 + v_n_buf5[pos5] * -g;
delta1 = v_n5 * g + v_n_buf5[pos5];
v_n_buf5[pos5] = v_n5;
pos5 += 1;
pos5 > buf_length1 ? pos5 = 0;
);

// signal highpass
spl0 -= (h2+=((spl0-h2)*cut2));
spl1 -= (h3+=((spl1-h3)*cut2));

// delta lowpass
delta0 += (l2+=((delta0-l2)*cut2));
delta1 += (l3+=((delta1-l3)*cut2));

// saturation
spl0 = atan(spl0 * delta0);//  / (spl0 === 0 ? 1 : delta0);
spl1 = atan(spl1 * delta1);//  / (spl1 === 0 ? 1 : delta1);
//spl0 = sin(spl0 * delta0) / (spl0 === 0 ? 1 : delta0);
//spl1 = sin(spl1 * delta1) / (spl1 === 0 ? 1 : delta1);
// gain compensation
spl0 /= adj_s3;
spl1 /= adj_s4;

)

// output level
spl0 *= vol;
spl1 *= vol;
);

*/

class RubidiumDistortion
{
public:
    RubidiumDistortion(juce::AudioProcessorValueTreeState &treeState) : saturation(treeState, "rubidiumAmount"),
                                                                        hysteresis(treeState, "rubidiumHysteresis"),
                                                                        tone(treeState, "rubidiumTone")
    {
    }

    void prepare(juce::dsp::ProcessSpec &spec)
    {
        srate = spec.sampleRate;
        dt = 1 / spec.sampleRate;

        maxBlockSize = spec.maximumBlockSize;

        dataBuffer.setSize(1, 70000);
        
        for (int i = 0; i < 70000; i++)
        {
            dataBuffer.setSample(0, i, 0);
        }

        updateCoefficients();
    }

    void updateCoefficients() {
        float satAmt = saturation.getRaw() * 0.9f + 10.f;
        float hystAmt = hysteresis.getRaw();
        float toneAmt = tone.getRaw();

        highpassFreq = toneAmt;

        float stepRatio = 2.0f;

        gain = 0.125 + (satAmt / 15);
        hys0 = 0.015625 + 0.015625 * (hystAmt / 12.5);
        hys1 = hystAmt / 100;
        cut0 = 2 * juce::MathConstants<float>::pi * (highpassFreq + rand0) / srate;
        cut1 = 2 * juce::MathConstants<float>::pi * (highpassFreq + rand1) / srate;
        cut2 = 2 * juce::MathConstants<float>::pi * (0.01f) / srate;
        cut2 = 0;
        buf_length1 = fmax((stepRatio * 2) - 1, 0);

        if (stepRatio == 6) buf_length1 = 21;

        buf_length0 = fmax(buf_length1 * buf_length1 * 2, 1);
        buf_length1 *= 2;

        if (buf_length1 == 2)
            buf_length0 = 6;

        vol = 0.9f;

        // smooth fader
        adj3 = gain <= 0 ? 0 : exp(shape * log10(gain));
        adj4 = gain <= 0 ? 0 : exp(shape * log10(gain));
        rc = 0.2 * fmax(128 * 0.001, maxBlockSize / srate);
        a = dt / (rc + dt);
    }

    void processBlock(juce::dsp::AudioBlock<float> &block)
    {
        updateCoefficients();

        for (int sample = 0; sample < block.getNumSamples(); sample++)
        {
            double spl0 = (double)block.getSample(0, sample);
            double spl1 = (double)block.getSample(1, sample);

            spl0 = atan(spl0 * 1.6);
            spl1 = atan(spl1 * 1.6);

            // smooth fader gain
            // adj_s3 += a * (adj3 - adj_s3);
            // if (!adj3 && !adj_s3)
            //     adj_s3 = adj3;
            // adj_s4 += a * (adj4 - adj_s4);
            // if (!adj4 && !adj_s4)
            //     adj_s4 = adj4;
            spl0 *= adj3;
            spl1 *= adj4;


            // delta signal hysteresis - single pole allpass
            ap1_x0 = spl0;
            ap1_y = ap1_y0 + ap1_k0 * ap1_x0;
            ap1_y0 = ap1_x0 - ap1_k0 * ap1_y;
            delta0 = ap1_y;
            ap1_x1 = spl1;
            ap1_z = ap1_y1 + ap1_k1 * ap1_x1;
            ap1_y1 = ap1_x1 - ap1_k1 * ap1_z;
            delta1 = ap1_z;


            // low level hysteresis - nested allpass
            double in0 = ap1_y + cDenorm;
            double v_n0 = in0 + dataBuffer.getSample(0, v_n_buf0 + pos0) * -g;
            double out0 = v_n0 * g + dataBuffer.getSample(0, v_n_buf0 + pos0);
            dataBuffer.setSample(0, v_n_buf0 + pos0, v_n0);
            pos0 += 1;
            if (pos0 > buf_length0)
                pos0 = 0;
            double in1 = ap1_z + cDenorm;
            double v_n1 = in1 + dataBuffer.getSample(0, v_n_buf1 + pos1) * -g;
            double out1 = v_n1 * g + dataBuffer.getSample(0, v_n_buf1 + pos1);
            dataBuffer.setSample(0, v_n_buf1 + pos1, v_n1);
            pos1 += 1;
            if (pos1 > buf_length0)
                pos1 = 0;
            delta0 += hys1 * fmax(hys0 - sqrt(out0), 0);
            delta1 += hys1 * fmax(hys0 - sqrt(out1), 0);

            // delta lowpass + rectified
            l0 += ((delta0 - l0) * cut0);
            delta0 = l0 * l0;
            l1 += ((delta1 - l1) * cut1);
            delta1 = l1 * l1;

            // signal highpass
            h0 += (spl0 - h0) * cut0;
            spl0 -= h0;
            h1 += (spl1 - h1) * cut1;
            spl1 -= h1;

            // saturation
            spl0 = atan(spl0 * delta0) / (spl0 == 0 ? 1 : delta0);
            spl1 = atan(spl1 * delta1) / (spl1 == 0 ? 1 : delta1);
            //spl0 = sin(spl0 * delta0) / (spl0 === 0 ? 1 : delta0);
            //spl1 = sin(spl1 * delta1) / (spl1 === 0 ? 1 : delta1);

            // nested allpass
            if (buf_length1 == 0) {
                double in2 = spl0 + cDenorm;
                double v_n2 = in2 + dataBuffer.getSample(0, v_n_buf2 + pos2) * -g;
                double out2 = v_n2 * g + dataBuffer.getSample(0, v_n_buf2 + pos2);
                dataBuffer.setSample(0, v_n_buf2 + pos2, v_n2);
                pos2 += 1;
                if (pos2 > buf_length1)
                    pos2 = 0;
                spl0 = out2;
                double in3 = spl1 + cDenorm;
                double v_n3 = in3 + dataBuffer.getSample(0, v_n_buf3 + pos3) * -g;
                double out3 = v_n3 * g + dataBuffer.getSample(0, v_n_buf3 + pos3);
                dataBuffer.setSample(0, v_n_buf3 + pos3, v_n3);
                pos3 += 1;
                if (pos3 > buf_length1)
                    pos3 = 0;
                spl1 = out3;

                // delta
                double in4 = delta0 + cDenorm;
                double v_n4 = in4 + dataBuffer.getSample(0, v_n_buf4 + pos4) * -g;
                delta0 = v_n4 * g + dataBuffer.getSample(0, v_n_buf4 + pos4);
                dataBuffer.setSample(0, v_n_buf4 + pos4, v_n4);
                pos4 += 1;
                if (pos4 > buf_length1)
                    pos4 = 0;
                double in5 = delta1 + cDenorm;
                double v_n5 = in5 + dataBuffer.getSample(0, v_n_buf5 + pos5) * -g;
                delta1 = v_n5 * g + dataBuffer.getSample(0, v_n_buf5 + pos5);
                dataBuffer.setSample(0, v_n_buf5 + pos5, v_n5);
                pos5 += 1;
                if (pos5 > buf_length1)
                    pos5 = 0;
            }

            // signal highpass
            h2 += ((spl0 - h2) * cut2);
            spl0 -= h2;
            h3 += ((spl1 - h3) * cut2);
            spl1 -= h3;

            // delta lowpass
            l2 += ((delta0 - l2) * cut2);
            delta0 += l2;
            l3 += ((delta1 - l3) * cut2);
            delta1 += l3;

            // saturation
            spl0 = atan(spl0 * delta0) / (spl0 == 0 ? 1 : delta0);
            spl1 = atan(spl1 * delta1) / (spl1 == 0 ? 1 : delta1);

            // gain compensation
            spl0 /= adj3;
            spl1 /= adj4;

            // output level
            spl0 *= vol;
            spl1 *= vol;

            block.setSample(0, sample, (float)spl0);
            block.setSample(1, sample, (float)spl1);
        }
    }

private:
    SmoothParam saturation;
    SmoothParam hysteresis;
    SmoothParam tone;

    juce::AudioBuffer<double> dataBuffer;

    double gain, hys0, hys1, cut0, cut1, cut2, buf_length0, buf_length1, vol = 0;

    double a, adj3, adj4, rc = 0;

    double highpassFreq = 5.0f; // highpass

    double maxBlockSize = 128;

    double ap1_x0 = 0;
    double ap1_y = 0;
    double ap1_x1 = 0;
    double ap1_z = 0;

    int pos0 = 0;
    int pos1 = 0;
    int pos2 = 0;
    int pos3 = 0;
    int pos4 = 0;
    int pos5 = 0;

    double delta0 = 0;
    double delta1 = 0;

    // smooth fader
    double shape = 4;
    double srate = 44100;
    double dt = 1 / srate;

    double adj_s3 = 1;
    double adj_s4 = 1;
    // nested allpass
    double cDenorm = 10 ^ -30;
    double v_n_buf0 = 1000;
    double v_n_buf1 = 2000;
    double v_n_buf2 = 3000;
    double v_n_buf3 = 4000;
    double v_n_buf4 = 5000;
    double v_n_buf5 = 6000;
    double g = 0;
    // highpass
    double h0 = 0;
    double h1 = 0;
    double h2 = 0;
    double h3 = 0;
    // lowpass
    double l0 = 0;
    double l1 = 0;
    double l2 = 0;
    double l3 = 0;
    // filter randomizer
    double rand0 = 0.02;
    double rand1 = 0.02;
    // single pole allpass
    double ap1_k0 = -0.9;
    double ap1_k1 = -0.9;
    double ap1_y0 = 0;
    double ap1_y1 = 0;
};