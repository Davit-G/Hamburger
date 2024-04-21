#pragma once

#include "juce_core/juce_core.h"
#include "juce_audio_processors/juce_audio_processors.h"

class RubidiumDistortion
{
public:
    RubidiumDistortion(juce::AudioProcessorValueTreeState &treeState) : drive(treeState, "rubidiumAmount"),
                                                                        hysteresis(treeState, "rubidiumAsym"),
                                                                        tone(treeState, "rubidiumTone"),
                                                                        mojo(treeState, "rubidiumMojo")
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

        mojo.prepare(spec);
        hysteresis.prepare(spec);
        drive.prepare(spec);
        tone.prepare(spec);

        updateCoefficients();

        float stepRatio = 2.0f;
        vol = 0.9f;

        buf_length1 = fmax((stepRatio * 2) - 1, 0);

        if (stepRatio == 6) buf_length1 = 21;

        buf_length0 = fmax(buf_length1 * buf_length1 * 2, 1);
        buf_length1 *= 2;

        if (buf_length1 == 2)
            buf_length0 = 6;
        
        rc = 0.2 * fmax(128 * 0.001, maxBlockSize / srate);
        a = dt / (rc + dt);
    }

    void updateCoefficients() {
        float satAmt = powf((mojo.getNextValue() * 0.9f + 10.0f) * 0.01f, 1.2f) * 100.0f + 5.0f;
        float hystAmt = powf(hysteresis.getNextValue() * 0.1f, 2.0f) * 300.f;
        float toneAmt = tone.getNextValue();

        highpassFreq = toneAmt;

        gain = 0.125 + (satAmt / 15);
        hys0 = 0.015625 + 0.015625 * (hystAmt / 12.5);
        hys1 = hystAmt / 100;
        cut0 = 2 * juce::MathConstants<float>::pi * (highpassFreq + rand0) * dt;
        cut1 = 2 * juce::MathConstants<float>::pi * (highpassFreq + rand1) * dt;
        cut2 = 0.f;

        // smooth fader
        adj3 = gain <= 0 ? 0.0000001f : exp(shape * log10(gain)) + 0.00000001f;
        adj4 = gain <= 0 ? 0.0000001f : exp(shape * log10(gain)) + 0.00000001f;
    }

    void processBlock(juce::dsp::AudioBlock<float> &block)
    {
        
        drive.update();
        mojo.update();
        hysteresis.update();
        tone.update();

        for (int sample = 0; sample < block.getNumSamples(); sample++)
        {
            updateCoefficients();

            double spl0 = (double)block.getSample(0, sample);
            double spl1 = (double)block.getSample(1, sample);

            float driveKnob = drive.getNextValue() * 0.01f; // normalised between 0 - 1
            float driveVal = juce::Decibels::decibelsToGain(driveKnob * 30.0f); 

            spl0 = atan(spl0 * 1.3 * driveVal);
            spl1 = atan(spl1 * 1.3 * driveVal);

            spl0 *= adj3;
            spl1 *= adj4;

            // delta signal hysteresis
            ap1_x0 = spl0;
            ap1_y = ap1_y0 + ap1_k0 * ap1_x0;
            ap1_y0 = ap1_x0 - ap1_k0 * ap1_y;
            delta0 = ap1_y;
            ap1_x1 = spl1;
            ap1_z = ap1_y1 + ap1_k1 * ap1_x1;
            ap1_y1 = ap1_x1 - ap1_k1 * ap1_z;
            delta1 = ap1_z;

            // low level hysteresis
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

            double out0sign = out0 < 0 ? 0 : 1;
            double out1sign = out1 < 0 ? 0 : 1;

            delta0 += hys1 * fmax(hys0 - sqrt(out0 * out0sign) * out0sign, 0.0);
            delta1 += hys1 * fmax(hys0 - sqrt(out1 * out1sign) * out1sign, 0.0);

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
            spl0 = atan(spl0 * delta0) / (spl0 == 0 ? 1 : (delta0 == 0 ? 0.00000000000001 : delta0));
            spl1 = atan(spl1 * delta1) / (spl1 == 0 ? 1 : (delta1 == 0 ? 0.00000000000001 : delta1));

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
            
            spl0 = atan(spl0 * delta0) / (spl0 == 0 ? 1 : (delta0 == 0 ? 0.0000000000001 : delta0));
            spl1 = atan(spl1 * delta1) / (spl1 == 0 ? 1 : (delta1 == 0 ? 0.0000000000001 : delta1));
            
            spl0 /= adj3 * (driveVal * 0.1f + 1.0f);
            spl1 /= adj4 * (driveVal * 0.1f + 1.0f);
            
            spl0 *= vol;
            spl1 *= vol;

            block.setSample(0, sample, (float)spl0);
            block.setSample(1, sample, (float)spl1);
        }
    }

private:
    SmoothParam drive;
    SmoothParam mojo;
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

    int v_n_buf0 = 1000;
    int v_n_buf1 = 2000;
    int v_n_buf2 = 3000;
    int v_n_buf3 = 4000;
    int v_n_buf4 = 5000;
    int v_n_buf5 = 6000;

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