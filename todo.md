
# Must

support mono / stereo only, dont have it break on mono
performance optimisations
bugfixes
emphasis coefficient interpolation with smoothed values
remove sentry.io before v1.0 (was useless)

# Should

finish matrix distortion section with more distortions
speed up DSP by replacing some direct form filters with svf ones (in the case of per-sample coefficient modulation) (or optionally utilise coefficient interpolation for IIR filters)
auval

# Would be nice

add more dist types (foil?? inherit from other plugins?)
icons in middle of saturation knobs with different cool things like tube, rubidium etc
other filters before / after distortion, such as combs / phasers / filters etc  
allpass stack in phase path for phase distortion (nah leave as is)
frequency shifter in phase path for phase distortion (is more interesting than allpass stack)
visualisations on top right for what the things are doing (phase visualiser for phase distortion plugin?)

# feedback 

complete manual page, reupload with new info for plugin (specify order of effect operations in manual or in plugin)
separate parameters that have common IDs in the apvts

# other
gain compensation on grunge?