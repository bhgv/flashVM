# Bounded native event voices

## v0.783: exact storage and preparation ceilings

[`fvm_swf_sound_voice_open_bounded`](../include/flashvm/swf_sound.h#L97) retains the existing PCM/ADPCM/MP3, range,
loop and envelope behavior of [`fvm_swf_sound_voice_open`](../include/flashvm/swf_sound.h#L82). Its additional budget
covers the native object, exact selected-interval S16 array and copied envelope
records. Check addition/multiplication before allocating any of these owners;
even an empty voice needs its native object. Zero or insufficient budget returns
OUT_OF_MEMORY and null output. Runtime payload limits still apply independently.
Codec/source errors propagate. Decoder stack, VM allocations, allocator overhead,
process RSS and output queues are excluded. The ceiling bounds storage, not CPU.

[`fvm_swf_sound_voice_storage_info`](../include/flashvm/swf_sound.h#L101) copies exact retained/PCM/envelope charges
without allocating, callbacks or source access. Caller sets exact struct_size;
invalid arguments preserve output. The voice remains independently owned after
reload, GC and context teardown. All operations require external serialization;
source preparation runs on the context owner. Close with voice_close.

Tests: `swf_sound_voice_storage` checks exact and one-byte-short boundaries for
six independently generated MPEG geometries and mono/stereo PCM, output lifetime,
null/wrong-size arguments and unchanged query output. These focused checks do
not certify complete SWF sound, reference timing or physical device behavior.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_voice_open` | function | [include/flashvm/swf_sound.h:82](../include/flashvm/swf_sound.h#L82) |
| `fvm_swf_sound_voice_open_bounded` | function | [include/flashvm/swf_sound.h:97](../include/flashvm/swf_sound.h#L97) |
| `fvm_swf_sound_voice_storage_info` | function | [include/flashvm/swf_sound.h:101](../include/flashvm/swf_sound.h#L101) |
<!-- source-links:end -->
