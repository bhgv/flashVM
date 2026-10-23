# Inferred partial ADPCM stream preparation


v0.1064: Add bounded PCM preparation that preserves canonical whole packets and accepts a unique zero-padded partial length without header averages.

v0.1065: Enumerate independent frame-count candidates for 104 geometries, checking every decoded sample and preserving ambiguous rejection and legacy whole packets.

v0.1066: Exercise all four native acquisitions, exact and short budgets, VM teardown and allocation-free immutable PCM reads.
