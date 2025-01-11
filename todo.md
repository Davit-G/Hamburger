# always remember

performance optimisations
bugfixes


# for v1.0

add source files for everything apparently

support mono / stereo only, dont have it break on mono

UX:
allow for overwriting presets with the same name in the user folder

make presets hehe (maybe change the suffix from borgir to some other suffix)


go back to the companding based distortion (the trick is to link the level on both segments)
rework matrix distortion into a more flexible waveshaping variant

speed up dsp performance by replacing some direct form filters with svf ones (emphasis coefficient interpolation with smoothed values) (in the case of per-sample coefficient modulation) (or optionally utilise coefficient interpolation for IIR filters) (OR JUST DONT RECOMPUTE COEFFICIENTS? that's not great for automation though)

auval

remove ai image stuff and fill it in with something else

# Would be cool


add more dist types (foil?? inherit from other plugins?)
tape hysteresis???? 

modulation options (lfo, etc)

user driven theming / customisation (either via png or svg)

icons in middle of saturation knobs with different cool things like tube, rubidium etc (maybe not?)

phase dist shift: provide more dynamic range for subtle movement
phase dist: remove clipping artifacting when input is too high (as a result of losing information when going backward in phase distortion)


# feedback 

complete manual page, reupload with new info for plugin (specify order of effect operations in manual or in plugin)

# other
gain compensation on grunge?

# not a priority (no)

freq shift: use polyphase designer or pre-bake coefficients (not needed)
other filters before / after distortion, such as combs / phasers / filters etc  (nah plugin will do too much)
allpass stack in phase path for phase distortion (nah leave as is)
visualisations on top right for what the things are doing (phase visualiser for phase distortion plugin?)







never stops creeping with scope