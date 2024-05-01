
# Must

support mono / stereo only, dont have it break on mono
performance optimisations
auval
emphasis coefficient interpolation with smoothed values

# Should

fill in matrix distortion section with more distortions
speed up DSP by replacing some direct form filters with svf ones (in the case of per-sample coefficient modulation)

# Would be nice

add more dist types (foil??)
icons in middle of saturation knobs with different cool things like tube, rubidium etc
other filters before / after distortion, such as combs / phasers / filters etc  
self hosted sentry.io?
allpass stack in phase path for phase distortion
frequency shifter in phase path for phase distortion

# feedback 

complete manual page, reupload with new info for plugin (specify order of effect operations in manual or in plugin)
remove perfetto completely from git fetches on release builds
separate parameters that have common IDs in the apvts

# other
gain compensation on grunge?