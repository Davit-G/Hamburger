
# Must

support mono / stereo only, dont have it break on mono
performance optimisations
bugfixes
emphasis coefficient interpolation with smoothed values

still needs ui bug fixing where listbox component duplicates

UX:
allow for overwriting presets with the same name in the user folder
Autofill the name of the preset as well as the author into the save box if a preset is currently selected

# Should

go back to the companding based distortion?
finish matrix distortion section with more distortions
speed up DSP by replacing some direct form filters with svf ones (in the case of per-sample coefficient modulation) (or optionally utilise coefficient interpolation for IIR filters)
auval

# Would be nice

add more dist types (foil?? inherit from other plugins?)
tape hysteresis????


icons in middle of saturation knobs with different cool things like tube, rubidium etc

phase dist shift: provide more dynamic range for subtle movement
phase dist: remove clipping artifacting when input is too high
freq shift: use polyphase designer or pre-bake coefficients


# feedback 

complete manual page, reupload with new info for plugin (specify order of effect operations in manual or in plugin)

# other
gain compensation on grunge?

# not a priority (no)

other filters before / after distortion, such as combs / phasers / filters etc  (nah plugin will do too much)
allpass stack in phase path for phase distortion (nah leave as is)
visualisations on top right for what the things are doing (phase visualiser for phase distortion plugin?)