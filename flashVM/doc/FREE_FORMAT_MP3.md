# Free-format MPEG Layer III in SWF

[Русский](FREE_FORMAT_MP3.ru.md).

## Strict framing (v0.806)

The private [fvm_swf_mp3_probe](../src/swf/sound_mp3.cpp#L52) accepts bitrate index zero when three consecutive
matching headers establish the smallest verified packet span at offset zero. It never skips
junk to synchronize. Rate, sample count and channel count must match; padding adds
one byte independently per packet. The exclusive packet-span ceiling is 2304 bytes,
matching the pinned minimp3 backend. Insufficient synchronization evidence is UNSUPPORTED; malformed headers are INVALID_ARGUMENT. Output metadata is untouched on errors.
No allocation, device, VM access or callback is involved.

The fixtures are original analytical signals encoded by system LAME 3.100 in
free-format mode, with independent native S16 values decoded by libmpg123.
The runtime still uses the existing static scalar minimp3; no new runtime library
or platform requirement is introduced. Device acceptance remains separate.


## Event decoding (v0.807)

DefineSound uses the inferred first packet size to initialize the pinned native
decoder, then retains the unpadded span and bit reservoir through the last packet.
The complete source is validated before writing selected PCM, including zero-length
reads. Initial latency, range limits, geometry and allocation-free two-pass reads
retain their existing contracts. One/two-packet standalone events without sufficient
header evidence remain UNSUPPORTED; their frame size cannot be learned from bitrate zero.


## Counted stream blocks (v0.808)

A SoundStreamBlock provides an exact UI16 decoded sample count. Before its first
free-format decode, native preparation derives the packet count from MPEG geometry
and checks candidate unpadded spans against every header and the complete block
extent. Exactly one candidate must consume the declared number of packets. This
allows a one-packet first block, two-packet blocks and larger groups without reading
across SWF prefixes. Empty blocks preserve history. Learned geometry/reservoir persist
across subsequent blocks; per-block decoded counts still have to match exactly.
No extra allocation or retained cache is added; raw/PCM/peak budget contracts remain.


## Bounded inference work (v0.809)

Counted-block inference considers at most two unpadded spans: floor(bytes/packets)
and its predecessor, since each packet contributes at most one padding byte.
It validates each header and exact terminal extent before committing decoder state.
Truncation, nonmultiples, zero counts with bytes and counts beyond UI16 are rejected.
Grouped-packet and failure-retry tests cover all four fixtures. Allocation-failure
sweeps confirm unchanged preparation acquisitions; copied PCM survives VM teardown
and reads/closes with allocation completely disabled.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_mp3_probe` | function | [src/swf/sound_mp3.cpp:52](../src/swf/sound_mp3.cpp#L52) |
<!-- source-links:end -->
