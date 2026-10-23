# Prepared MP3 frame seeks

MP3 preparation retains each block's original decoded prefix and signed
SeekSamples before discarding encoded bytes. Preparation performs two complete
passes with a persistent reservoir per pass; the resulting PCM trims initial
latency once. The immutable native owner survives VM destruction.

For a later authored block, let R be its original decoded prefix and S its signed
SeekSamples. Adobe's SWF specification (pages 185–187) adds initial latency L
on discontinuous seeks: raw position R+L+S. In the already trimmed snapshot this
is R+S. The first block starts at normalized zero. Continuous playback remains
unchanged. Reference: [Adobe SWF specification](https://open-flash.github.io/mirrors/swf-spec-19.pdf).

Negative initial latency, full reference A/V scheduling, decoder read-ahead,
child streams and physical device acceptance remain open. All nine roadmap
stages remain open. Version numbers do not certify SWF 8 completeness.
