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

## v0.784: reusable prepared PCM

[`fvm_swf_sound_voice_clone`](../include/flashvm/swf_sound.h#L111) makes a deep independent copy of a prepared voice,
including its current cursor, repetition and envelope segment, or EOF. It never
re-enters a context or decoder. The explicit budget includes the object and exact
arrays. On failure the source is unchanged and output is null. Keep source and
output ownership storage disjoint and serialize the source through return.
After success the two owners share nothing mutable; distinct workers may read
and close them independently. This deliberately copies PCM rather than adding a
shared cross-thread reference count. A host can keep a template after VM teardown
and clone it for overlap without repeating MP3/ADPCM decode. Memory is not free:
charge every template and clone. Close each untransferred result separately.

`swf_sound_voice_clone` compares original/copy after partial reads, repeats,
envelope changes and EOF, then destroys the original and continues the copy.
Exact/short budgets and argument preservation are exercised after source teardown.

## v0.785: native cursor positioning

[`fvm_swf_sound_voice_seek`](../include/flashvm/swf_sound.h#L123) selects a repeat and source-relative frame without
allocation or decoding. For a nonempty voice choose play_index < play_count and
frame < frames_per_play, or (play_count,0) for EOF. Empty/zero-play voices accept
only (0,0), remaining finished. Invalid input leaves all progress unchanged.
(0,0) restarts a nonempty template. The next read reconstructs envelope time from
the absolute elapsed repeats; backward seeks reset cached envelope lookup.
Serialize the caller-owned voice. Never seek an already transferred handle.
This positions prepared source PCM, not a movie timeline, conversion grid, mixer,
pending queue block or hardware clock. Those owners need their own reset policy.

`swf_sound_voice_seek` compares every position in a three-repeat, three-envelope
fixture with a previously rendered independent sequential result, in reverse
order after VM destruction. It checks restart, canonical EOF, invalid unchanged
progress, empty and zero-loop owners with allocation forbidden throughout seeks.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_voice_clone` | function | [include/flashvm/swf_sound.h:111](../include/flashvm/swf_sound.h#L111) |
| `fvm_swf_sound_voice_open` | function | [include/flashvm/swf_sound.h:82](../include/flashvm/swf_sound.h#L82) |
| `fvm_swf_sound_voice_open_bounded` | function | [include/flashvm/swf_sound.h:97](../include/flashvm/swf_sound.h#L97) |
| `fvm_swf_sound_voice_seek` | function | [include/flashvm/swf_sound.h:123](../include/flashvm/swf_sound.h#L123) |
| `fvm_swf_sound_voice_storage_info` | function | [include/flashvm/swf_sound.h:101](../include/flashvm/swf_sound.h#L101) |
<!-- source-links:end -->
