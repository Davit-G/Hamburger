# For v0.4

double check all knob ranges are reasonable
fill in matrix distortion section with more distortions

# Must

support mono / stereo only, dont have it break on mono
performance optimisations
pluginval
make all parameters smooth to remove clicking

# Should

speed up DSP by replacing some direct form filters with svf ones (in the case of per-sample coefficient modulation)

# Would be nice

add more dist types (foil??)
icons in middle of saturation knobs with different cool things like tube, rubidium etc
other filters before / after distortion, such as combs / phasers / filters etc
self hosted sentry.io?
sentry.io proper pdb / breakpad minidump files to make symbols readable, or memory dump. might be too much for free plugin?

# feedback 

allpass is mixing dry signal (?)
correct oversampling delay compensation
complete manual page, reupload with new info for plugin
specify order of effect operations in manual or in plugin
remove melatonin inspector and perfetto completely from git fetches on release builds
separate parameters that have common IDs in the apvts


# other
gain compensation on grunge?

Revert to Release and not RelWithDebInfo once v1.0 is out