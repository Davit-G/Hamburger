# Hamburger Audio Plugin
If you are a musician who wants to use this plugin, please download the binaries from [my store page](https://ko-fi.com/s/6e7ded34dc).

Hamburger is a distortion plugin with inbuilt dynamics controls and equalisation that can deliver both subtle tangy harmonics and absolute annihilation and noise-wall-ification to any sound. 

<p align="center">
<img width="800" alt="Image of Hamburger Plugin, with tube distortion" src="https://github.com/user-attachments/assets/ecffa533-a8ad-4ba7-80a1-d26988caefd9" />
</p>

If you need control over your distortion tone, you can carve and refine your sound resulting in extremely unique timbres. Compress and shape the dynamics of your sound before distorting to make your synths and instruments punchy and juicy. You can provide subtle flare, texture and grit to a sound that didn't have one. You can also hit the pedal to the metal, go crazy with the knobs and enter sonic territories that other distortion plugins would be TERRIFIED to enter. 

# Download

Hamburger is supported for Windows, Linux (x86_64) and both Apple Silicon and Intel MacOS devices, in VST3 / CLAP / AU / LV2 formats.

Store page (Download free builds)
- https://ko-fi.com/s/6e7ded34dc

Main page
- https://aviaryaudio.com/plugins/hamburgerv2

## Audio demos

[Check out audio demos here](https://soundcloud.com/davz-razorblades/hamburger-guitar-demo-dry-wet). More audio demos coming soon

# Overview

- Several distortion modules:
    - Grill (normal saturation with asymmetry and wavefolding)
    - Tube (class A)
    - Phase Distortion (for watery tones)
    - Rubidium Distortion (transformer saturation based on some old ReaJS code)
    - Matrix Distortion (waveshaping)
    - Tape Distortion (Preisach tape hysteresis model)
    - Slew Distortion (Slew based limiting used as distortion)
    - Clipper at the end of the chain to help with controlling unnecessary peaks, and for your own safety :D

- Pre-filtering effects
    - Smooth and responsive allpass chain to add phase shifting to signal
    - Grunge (resonant filtering). Good on guitars, makes the distortion more grimey.

- Adjust dynamics before distorting:
    - Stereo Compressor (with adjustable stereo link to create a dual mono compressor)
    - Multiband (with optional downwards compression) 
    - Mid/Side Compressor (for taming arbitrary stereo content)

- Add grit and noise to the signal with:
    - Sizzle (tries to impart white noise into zero crossings of signal)
    - Digital Erosion (inspired by Ableton's Erosion effect)
    - Downsampling and Bit Reduction (digital bitcrush)
    - Hard Gate (harsh and loud)
    - Fizz (white noise layer)

- 2 Band Emphasis EQ to tone and shape the signal before and after distortion
- Option to duplicate current distortion layer (stacking up to 8x) for even more powerful and dynamic behaviour
- Oversampling support to reduce aliasing artifacts

## AI Disclosure ⚠️

Hamburger is not vibe coded (this repository has development history prior to the widespread use of AI). With that being said, there is AI usage. AI has been recently used for cross platform debugging as well as some GUI code. None of the signal processing is AI generated, and any generated code is heavily reviewed and modified / cleaned up or rewritten before version releases. 
Despite AI usage, this code is still majority written by me.

My end goal with Hamburger is to have something that I can personally rely on in my own music. I cannot be confident that vibe coding can create reliable tools in this way. If I am not happy using my own tools, why should anyone else use it?

Having AI usage here is admittedly embarrassing considering that I hate it with a passion and hate what it's done to the world. My career was affected by it (my previous workplace forced AI driven development to the point where I often burned out), and seeing the massive vibe coded plugin slop tsunami online doesn't raise my spirits. I can only hope that despite this, this plugin speaks for itself and you still find it useful. I understand if you don't want to use my stuff as a result.

## Building

To build Hamburger from source, you'll need:
- CMake 3.15 or later
- A C++17 compatible compiler
- Git

### Quick Build Instructions

1. Clone the repository:
```bash
git clone https://github.com/Davit-G/Hamburger.git
cd Hamburger
```

2. Configure step:
```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
```

3. Build the project:
```bash
cmake --build build --config Release
```

The built plugins will be automatically copied to:
- **macOS**: `../build_mac/`
- **Linux**: `../build_linux/`
- **Windows**: `../build_windows/`
