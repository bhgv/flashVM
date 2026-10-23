# SWF MP3 audio

Prepared MP3 discontinuities now have a checked signed-position path:
[authored frame seek](MP3_FRAME_SEEK.md). It preserves continuous decoding and
adds transactional native/desktop navigation. Full reference read-ahead/A/V
pacing and negative initial latency remain separate open limits.


[Русский](MP3_AUDIO.ru.md) · [functional roadmap](ROADMAP.md)

Current v0.819: [free-format decoding](FREE_FORMAT_MP3.md) and protected-region
[CRC validation](MP3_CRC.md) are implemented. Historical checkpoint limitations
below are superseded by these contracts. Short standalone events without framing
evidence, negative latency, complete Huffman integrity and reference seek/A/V
remain outside this support. MPEG CRC covers header/side information, not main data.

## v0.776 scalar decoder foundation

A pinned, unmodified [minimp3](../external/minimp3/README.md) Layer-III decoder is
compiled into its own static core dependency, even with NO_MEDIA. Symbols have
private prefixes; scalar C++98 code avoids ISA-dependent dispatch, containers,
allocations, VM references, callbacks or devices. The public host ABI remains C89.
This first checkpoint does not yet route public SWF resource reads to the decoder.

[fvm_swf_mp3_probe](../src/swf/sound_mp3.cpp#L94) validates a complete contiguous MPEG-1/2/2.5 Layer-III packet
before exposing its exact bytes/rate/channels/576-or-1152 frame count. It rejects
reserved fields, short packets and synchronization gaps; Layer I/II and free-format
bitrate return UNSUPPORTED. CBR and variable standard bitrate indices are allowed.
[FVM_SWFMP3Decoder](../src/swf/sound_mp3.hpp#L29) retains reservoir/filter history across packets and publishes PCM
only after matching exact geometry. A failed decode is terminal for that decoder;
no search over junk bytes or missing-reservoir concealment is presented as success.
CRC verification, free-format bitrate inference and complete corrupt-Huffman
validation are not claimed. Separate instances own all mutable state.

The decoder fixture checks three real analytical MP3 vectors: MPEG-2.5 mono 11025,
MPEG-2 stereo 22050 and MPEG-1 stereo 44100 Hz. Selected samples match independent
FFmpeg decoding within two S16 units; complete packet/sample counts, header bounds
and rejected reserved/free-format fields are checked. The checked-in vectors build
offline. scripts/generate_mp3_test_fixture.py uses FFmpeg/libmp3lame only to regenerate
analytical assets and independent golden samples, never during a normal build.

```sh
cmake --build build --target flashvm_tests_swf_mp3_decoder
ctest --test-dir build --output-on-failure -R swf_mp3_decoder
python3 scripts/check_vendor_sources.py
```

MP3 integration, event/stream scheduling, seeking and native device/platform
acceptance are separate from this decoder checkpoint. All roadmap stages remain open.


## v0.777 DefineSound decoding

[fvm_swf_sound_read_pcm](../include/flashvm/swf_sound.h#L148) now decodes MP3 in SWF4–8 without allocations or callbacks.
Its range is relative to the declared source after a nonnegative SI16 initial
SeekSamples prefix. Every complete packet is validated before any output changes;
then a fresh stack decoder copies only the requested range. Declared frames must
fit decoded data after latency; complete encoder tail packets are validated but
not exposed beyond that count. No hidden LAME/Xing gapless adjustment is applied.
Zero-length reads still validate the complete source. Source changes/teardown and
output buffers need owner serialization; no managed pointer survives return.

[fvm_swf_sound_info](../include/flashvm/swf_sound.h#L134) uses effective first-packet MPEG rate/channels for supported
MP3 when that packet validates, preserving the original encoded bytes and source
count. Advisory SWF flags cannot silently resample the data. Opaque malformed
metadata remains inspectable, but decode rejects it. Negative latency, free-format
bitrate and other MPEG layers remain UNSUPPORTED. Missing/truncated packets,
changed geometry and too-large declared count are INVALID_ARGUMENT; bit/ownership
arithmetic overflow is OUT_OF_MEMORY. Native S16 representation restrictions still
apply. Repeated small reads redo full-source work: prepare a voice for playback.

The public fixture checks all versions 4–8, three MPEG generations, exact golden
positions before/after latency, wrong capacity, missing tail, preserved output and
SWF3 rejection. Existing PCM/event edge checks remain unchanged and pass.

Vendor validation: the new minimp3 header/LICENSE hashes match their pin. The
repository-wide vendor checker stops on a pre-existing AGG agg_array.h hash
mismatch; that file is byte-identical to the published v0.775 base and is not
changed by this series. A global vendor-manifest pass is not claimed.

## v0.778 MP3 event styles and native voices

SOUNDINFO fixed-44100-Hz in/out positions now use checked integer decomposition
for effective MPEG rates, including 32/48 kHz. Overflow returns INVALID_ARGUMENT
before source extraction or voice publication instead of wrapping on a native
32-bit unsigned long. Both whole-event channel rendering and incremental voices
use the same checked conversion; ordinary nominal PCM behavior is unchanged.

The fixture matrix now includes six MPEG geometries, 8000–48000 Hz, CBR and VBR.
Native voices apply in/out intervals, two repeats and quarter-gain envelopes after
initial MP3 latency. Converted stereo mixing uses independent rational sample
positions. Actual GC runs before source teardown; voices/mixers then continue
without a context. Golden MPEG samples, full declared intervals and exhausted
native voice lifetime are checked. Decoder/public-read tests remain passing.
This does not implement a general stream/event timeline or reference seek clock.

## v0.779 bounded continuous MP3 snapshots

[fvm_swf_stream_sound_snapshot_open_pcm](../include/flashvm/swf_stream_sound.h#L87) and its bounded counterpart now normalize
root MP3 segments to immutable PCM_NATIVE/S16. Complete Layer-III packets stay in
source block order and one reservoir/filter state spans all blocks, including
explicit empty markers. Each wire SampleCount must equal that block's complete
decoded MPEG sample count. Stable effective rate/channels replace advisory flags.
A nonnegative header latency is skipped once across as many initial packets as
needed; the first block SeekSamples must match it. Later SeekSamples values do not
trim continuous playback. Negative initial latency and free-format remain unsupported.

The existing snapshot budget includes object/index, encoded bytes and all decoded
PCM at peak. Validation computes exact normalized per-block lengths before the
single PCM allocation; a fresh native decoder writes the unpublished result.
Source bytes are released on success; every failure closes temporary ownership.
Native stack decoder/scratch, allocator bookkeeping and caller buffers are excluded
from these byte charges. Runtime payload limits and explicit peak ceilings both
apply. Existing immutable reader/converter APIs work after VM teardown; live/raw
MP3 inspection stays encoded. Empty blocks retain frame and ordinal identity.

The six-geometry matrix checks CBR/VBR, latency 0/1105 (including multi-packet
trimming), all selected independent golden values, empty markers, incorrect counts,
truncated packets and exact/one-byte-short peak ceilings. Allocation budgets 0–4
exercise every acquisition and prove allocation-free native reads after source
teardown. The prior counted ADPCM and storage fixtures still pass.

Discontinuous frame seeks retain the existing prefix-discard PCM policy: this
checkpoint does not retain/reapply original per-block MP3 seek corrections or
implement reference Flash read-ahead/A/V synchronization. Those limitations are
not removed by successful native decoding. MP3 is described by Adobe's
[SWF specification, sound chapter](https://open-flash.github.io/mirrors/swf-spec-19.pdf);
the implementation acceptance above is deliberately narrower than full Flash playback.

## v0.780 SDL player integration

The desktop root-stream preflight now accepts the supported MP3 subset alongside
PCM and whole-packet ADPCM. Every segment is completely decoded under the existing
aggregate preparation ceiling before window/device startup. The worker receives
immutable native PCM, not a VM or MPEG decoder. Root StartSound mixed with streams
is still rejected; event-only MP3 uses the existing native event output path.
Help and source contracts list both routes and their limits.

The real SDL queue trace is checked against independent FFmpeg/ffprobe packet
boundaries and decoded samples, followed by an independent rational 48000-Hz
stereo conversion. Six rates/channel combinations, CBR/VBR, FWS/CWS, latency 0/529,
actual interpreter/native-JIT action counters and MP3-to-PCM header transitions
pass. Maximum accepted-output deviation is three S16 units; output lengths must
match exactly. A finite one-frame run excludes all unauthorized future blocks.
The optional CTest oracle explicitly skips with 77 if FFmpeg/ffprobe or native JIT
are unavailable; no such skip occurred on this Linux x64 validation host.

The existing real native worker fixture now includes MP3: slow owner work, frame
gates, clip stop/play, GC safe points, StopSounds cancellation, host pause,
same-frame navigation tokens, cross-header seeks and joined clear are exercised
in interpreter/JIT modes. Six worker/preparation/budget/refill checks pass. SDL
uses its dummy audio/video drivers: this is real queue/runtime evidence, not
physical listening, a DAC clock or foreign OS/device acceptance.

Initial latency trimming and prefix-discard seeks are exactly the native snapshot
policy above. Full reference MP3 seek/read-ahead, negative initial latency,
free-format bitrate, child streams, stream/event composition and A/V synchronization
remain open. No roadmap stage is marked complete.

## v0.781 executable C89 MP3 exporter

The [annotated tutorial](../examples/multimedia/swf_mp3/README.md) exports an actual
first MP3 StartSound or first root MP3 stream after source context/runtime teardown.
It bounds input, native preparation and export duration separately, copies native
S16 in fixed chunks and treats native file errors as terminal. No device, worker,
resampling, external codec SDK or source VM survives in the export loop.

Its actual executable passes ordinary strict C89 and full-minimal-core ASan/UBSan
in both modes, all six MPEG geometries and FWS/CWS. Independent golden positions,
complete output lengths, replacement, malformed CLI/source and terminal file errors
pass; with the available FFmpeg oracle every exported sample is compared within
three S16 units. Checked-in golden checks still run without FFmpeg. LeakSanitizer
is disabled on this host because process enumeration is unavailable; ASan injected
exception-unwind runs remain outside accepted evidence as documented previously.

## v0.782 prepared mixer commands without a source VM

[fvm_swf_sound_mixer_submit_prepared](../include/flashvm/swf_sound_mixer.h#L88)(mixer, voice) accepts a pointer to one live,
exclusively owned native voice handle. On OK it consumes ownership and clears the
handle; finished/NoMultiple no-ops close the unused owner. On error it preserves
handle/progress and every existing slot. Null arguments are INVALID_ARGUMENT,
strict geometry mismatch UNSUPPORTED and full capacity OUT_OF_MEMORY. NoMultiple
checks active non-releasing same-ID voices before rendering policy, matching
legacy submission. Adoption starts conversion phase zero at the CURRENT voice
cursor, never rewinds a partially read interval. No allocation, callback, VM or
managed root occurs. Keep owner storage disjoint and externally serialize access;
never use or close a handle after transfer.

[fvm_swf_sound_mixer_stop_id](../include/flashvm/swf_sound_mixer.h#L120)(mixer, sound_id) applies the existing immediate or
DECLICK policy without a context. ID must be 1–65535; null/invalid returns
INVALID_ARGUMENT before mutation, absent identity is OK. Repeated release stops
do not restart the fade. Stop/adoption affect future composition only: retained
mixed retry bytes and accepted device PCM are immutable until separately
acknowledged queue cancellation. These calls are not internal locks or a complete
frame-command scheduler. Legacy context submission delegates to the same native
control implementation after owner-side preparation.

The allocation-injected fixture forbids all new allocations after GC and source
teardown, covering transfer, full/mismatched preservation, duplicate/empty owner
consumption, partial-progress adoption, exact sample continuation, invalid/missing
IDs and immediate/repeated fade stop. The C89 MP3 tutorial now creates its mixer
and adopts ownership AFTER VM teardown, then exports exactly its explicit finite
voice duration. Its independent golden/full-FFmpeg oracle remains unchanged.
These are static-core C APIs; automatic stream/event desktop scheduling remains open.

Final v0.782 validation: 70 focused native/C89 tests and seven actual SDL dummy-player tests passed. Full-core ASan/UBSan success paths, including allocation-forbidden native transfer, passed; LeakSanitizer was disabled because this host cannot access /proc. Injected allocation failures 0–4 passed ordinary runs and scoped UBSan; exception unwinding is not claimed under ASan. Physical devices and other platform execution remain separate acceptance work.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_mp3_probe` | function | [src/swf/sound_mp3.cpp:94](../src/swf/sound_mp3.cpp#L94) |
| `fvm_swf_sound_info` | function | [include/flashvm/swf_sound.h:134](../include/flashvm/swf_sound.h#L134) |
| `fvm_swf_sound_mixer_stop_id` | function | [include/flashvm/swf_sound_mixer.h:120](../include/flashvm/swf_sound_mixer.h#L120) |
| `fvm_swf_sound_mixer_submit_prepared` | function | [include/flashvm/swf_sound_mixer.h:88](../include/flashvm/swf_sound_mixer.h#L88) |
| `fvm_swf_sound_read_pcm` | function | [include/flashvm/swf_sound.h:148](../include/flashvm/swf_sound.h#L148) |
| `fvm_swf_stream_sound_snapshot_open_pcm` | function | [include/flashvm/swf_stream_sound.h:87](../include/flashvm/swf_stream_sound.h#L87) |
| `FVM_SWFMP3Decoder` | struct | [src/swf/sound_mp3.hpp:29](../src/swf/sound_mp3.hpp#L29) |
<!-- source-links:end -->
