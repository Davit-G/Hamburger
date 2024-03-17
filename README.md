# Hamburger Audio Plugin

Hamburger is a distortion plugin with inbuilt dynamics controls and equalisation that can deliver both subtle tangy harmonics and absolute annihilation and noise-wall-ification to any sound. 

<p align="center">
<img width="800" alt="image" src="https://github.com/Davit-G/Hamburger/assets/36148073/109b3864-3230-418a-998b-2a6acf595fbe">
</p>

If you need control over your distortion tone, you can carve and refine your sound resulting in extremely unique timbres. Compress and shape the dynamics of your sound before distorting to make your synths and instruments punchy and juicy. You can provide subtle flare, texture and grit to a sound that didn't have one. You can also hit the pedal to the metal, go crazy with the knobs and enter sonic territories that other distortion plugins would be TERRIFIED to enter.

https://aviaryaudio.com/plugins/hamburgerv2

Audio demos coming soon :D

## Overview

## Features

- Several distortion modules:
    - Saturation (with adjustable bias)
    - Tube
    - Fuzz
    - Grunge (resonant)
    - Cooked (wavefolding)
    - Clipper at the end of the chain to help with controlling unnecessary peaks, and for your own safety :D

- Adjust dynamics before distorting:
    - Stereo Compressor (with adjustable stereo link to create a dual mono compressor)
    - Multiband (with optional upwards/downwards compression) 
    - Mid/Side Compressor

- Add grit and noise to the signal with:
    - Sizzle (white noise)
    - Digital Erosion
    - Downsampling and Bit Reduction
    - Jeff (?)

- 3 Band Emphasis EQ to tone and shape the signal before and after distortion
- Smooth and responsive allpass chain to add phase shifting to signal

## Building

Make sure CMake is configured to compile for your required platform.
JUCE is automatically included as part of the CMakeLists.txt definition, so no need to include JUCE as a subfolder.
