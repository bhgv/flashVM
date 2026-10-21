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
[FVM_SWFMP3Decoder](../src/swf/sound_mp3.hpp#L22) retains reservoir/filter history across packets and publishes PCM
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

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_mp3_probe` | function | [src/swf/sound_mp3.cpp:8](../src/swf/sound_mp3.cpp#L8) |
| `FVM_SWFMP3Decoder` | struct | [src/swf/sound_mp3.hpp:22](../src/swf/sound_mp3.hpp#L22) |
<!-- source-links:end -->
