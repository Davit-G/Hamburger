# always remember

performance optimisations
bugfixes

# feedback 

complete manual page, reupload with new info for plugin (specify order of effect operations in manual or in plugin)

lots of bugs with preset loading
- some presets from different versions claim to load but don't change anything
- after saving a new preset, only session presets are visible until plugin is reloaded
- no ability to overwrite existing presets with same name, might need new modal to fix issue
- preset names with OS-unsupported characters create corrupted files

in / out / mix might have to be moved somewhere else to avoid it being treated as it's own module, maybe a bottom bar with a slider?
users cant understand signal chain ordering, so either provide info in-plugin or via documentation
- maybe a little modal with info? or explanation to replace the ai images currently there on the side?

suggestion: dynamic audio reactive distortion controls which respond to the input?
make hamburger parameter values typable
convolution could be interesting but outside the scope of this plugin

# personal notes

diode in grill distortion isnt super useful?
tube tone knob is not super useful either I don't think
several stages of distortion would be very cool, probably not worth having options for separate stages just multiple of the same stage.
downsampling jitter could be cool
upwards comp on multiband?
matrix distortion is useless
50% mix on rubidium and tape sounds like its phase cancelling. phase distortion as well but cause thats literally phase shifting.
simd in more places, also look at FloatVectorOperations for simple things

# for v1.0

support mono / stereo only, dont have it break on mono
allow for overwriting presets with the same name in the user folder
make presets hehe (maybe change the suffix from borgir to some other suffix)

linear phase high pass filter option after everything?

tape hysteresis???? 
bias in rubidium

go back to the companding based distortion (the trick is to link the level on both segments)
- actually, companding is really hard to do right, would be cool to have but hamburger is not subtle enough for this to be useful

rework matrix distortion into a more flexible waveshaping variant
speed up dsp performance by replacing some direct form filters with svf ones (emphasis coefficient interpolation with smoothed values) (in the case of per-sample coefficient modulation) (or optionally utilise coefficient interpolation for IIR filters) (OR JUST DONT RECOMPUTE COEFFICIENTS? that's not great for automation though)
auval
remove ai image stuff and fill it in with something else

high / low filter in clipper section?

hysteresis from chowtape



# Would be cool

hamburger delay?
- time (ms, or bpm sync)
- feedback (-100%, 100%)
- offset (either opposite LR or same LR)
- mod (mod freq + mod amt)
- additional safety limiter

pnch for extra noise filter?

add source files for everything apparently

envelope follower filtered noise for the tapey lofi type sounds

add more dist types (foil?? inherit from other plugins?)

modulation options (lfo, etc) ?

user driven theming / customisation (either via png or svg)?

icons in middle of saturation knobs with different cool things like tube, rubidium etc (maybe not?)

phase dist shift: provide more dynamic range for subtle movement
phase dist: remove clipping artifacting when input is too high (as a result of not having information when going forward in phase distortion)
- might need some sort of buffering, and then delay compensation?

# other
gain compensation on grunge?

# not a priority (no)

freq shift: use polyphase designer or pre-bake coefficients (not needed)
other filters before / after distortion, such as combs / phasers / filters etc  (nah plugin will do too much?)
allpass stack in phase path for phase distortion (nah leave as is)
visualisations on top right for what the things are doing (phase visualiser for phase distortion plugin?)







never stops creeping with scope


# done (i made this pretty late so there's stuff i've deleted that is finished)

- author field: sometimes the author field isn't being saved