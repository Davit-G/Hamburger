# Hamburger

https://aviaryaudio.com/plugins/hamburgerv2

## Overview

Hamburger is a distortion plugin with inbuilt dynamics controls and equalisation that can deliver both subtle tangy harmonics and absolute annilhilation and noise-wall-ification to any sound.

## Features

- Several distortion modules:
    - Soft clipping (with adjustable bias)
    - Tube
    - Fuzz
    - Grunge (resonant distortion)
    - Cooked (wavefolding)

- Adjust dynamics before distorting:
    - Stereo Compressor (with adjustable stereo link to create a dual mono compressor)
    - Multiband (with optional upwards/downwards compression) 
    - Mid/Side Compressor
    - Compander (to be added soon)

- Add grit and noise to the signal with:
    - Sizzle (white noise)
    - Digital Erosion
    - Downsampling and Bit Reduction
    - Jeff (?)

- 3 Band Emphasis EQ to tone and shape the signal before and after distortion

- Smooth and responsive allpass chain to add phase shifting to signal

- SIMD optimised DSP for low CPU usage

## Building

Make sure JUCE modules and extras are included in a JUCE subfolder in the same directory before compiling.
Make sure CMake is configured to compile for your required platform.