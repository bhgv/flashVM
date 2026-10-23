# Inferred partial ADPCM stream preparation


v0.1064: Add bounded PCM preparation that preserves canonical whole packets and accepts a unique zero-padded partial length without header averages.

v0.1065: Enumerate independent frame-count candidates for 104 geometries, checking every decoded sample and preserving ambiguous rejection and legacy whole packets.

v0.1066: Exercise all four native acquisitions, exact and short budgets, VM teardown and allocation-free immutable PCM reads.

v0.1067: Check invalid endings and short predictors, ambiguous mono rejection, missing owners and a unique nonzero endpoint with exact PCM.

v0.1068: Use the inferred bounded snapshot policy for desktop stream segments while retaining aggregate peak budgets and preflight failure before device ownership.

v0.1069: Check FWS CWS interpreter JIT exact rational PCM, packet restart, empty blocks, PCM successor and rejection before device acquisition.

v0.1070: Real workers verify one-saturation stereo PCM and BASE19 TAIL0 for both finite policies, compressed SWF and both execution modes.

v0.1071: Add an inferred export policy without count-table access; verify the same eight hand-calculated samples in all three ownership modes.
