# SWF MP3 audio

[Русский](MP3_AUDIO.ru.md) · [functional roadmap](ROADMAP.md)

## v0.776 scalar decoder foundation

A pinned, unmodified [minimp3](../external/minimp3/README.md) Layer-III decoder is
compiled into its own static core dependency, even with NO_MEDIA. Symbols have
private prefixes; scalar C++98 code avoids ISA-dependent dispatch, containers,
allocations, VM references, callbacks or devices. The public host ABI remains C89.
This first checkpoint does not yet route public SWF resource reads to the decoder.

[fvm_swf_mp3_probe](../src/swf/sound_mp3.cpp#L8) validates a complete contiguous MPEG-1/2/2.5 Layer-III packet
before exposing its exact bytes/rate/channels/576-or-1152 frame count. It rejects
reserved fields, short packets and synchronization gaps; Layer I/II and free-format
bitrate return UNSUPPORTED. CBR and variable standard bitrate indices are allowed.
[FVM_SWFMP3Decoder](../src/swf/sound_mp3.hpp#L26) retains reservoir/filter history across packets and publishes PCM
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

[fvm_swf_sound_read_pcm](../include/flashvm/swf_sound.h#L108) now decodes MP3 in SWF4–8 without allocations or callbacks.
Its range is relative to the declared source after a nonnegative SI16 initial
SeekSamples prefix. Every complete packet is validated before any output changes;
then a fresh stack decoder copies only the requested range. Declared frames must
fit decoded data after latency; complete encoder tail packets are validated but
not exposed beyond that count. No hidden LAME/Xing gapless adjustment is applied.
Zero-length reads still validate the complete source. Source changes/teardown and
output buffers need owner serialization; no managed pointer survives return.

[fvm_swf_sound_info](../include/flashvm/swf_sound.h#L94) uses effective first-packet MPEG rate/channels for supported
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

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_mp3_probe` | function | [src/swf/sound_mp3.cpp:8](../src/swf/sound_mp3.cpp#L8) |
| `fvm_swf_sound_info` | function | [include/flashvm/swf_sound.h:94](../include/flashvm/swf_sound.h#L94) |
| `fvm_swf_sound_read_pcm` | function | [include/flashvm/swf_sound.h:108](../include/flashvm/swf_sound.h#L108) |
| `FVM_SWFMP3Decoder` | struct | [src/swf/sound_mp3.hpp:26](../src/swf/sound_mp3.hpp#L26) |
<!-- source-links:end -->
