# Finite mixed event tails

[`fvm_swf_sound_mixer_active_frames`](../include/flashvm/swf_sound_mixer.h#L54) queries a bounded remaining output horizon on the current mixer grid, including fractional lookahead, repeats and release. It does not consume PCM or infer physical audibility. All owners require external serialization; no allocation, callback, VM or GC occurs. maximum is 1..[FVM_SWF_SOUND_MIXER_MAX_HORIZON](../include/flashvm/swf_sound_mixer.h#L46); null/invalid input returns INVALID_ARGUMENT preserving out. Empty mixer returns OK with zero. The result is clamped to maximum and silent-valued source samples still count.

v0.1012: Add allocation-free active frame horizons including fractional lookahead, repeats and release without consuming PCM.


v0.1013: Check independent durations and PCM across four source clocks, six output rates, channel mappings, repetitions and partial source adoption.


v0.1014: Add opt-in horizon-limited tail pumping while preserving indivisible retries from every earlier rendering mode.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_mixer_active_frames` | function | [include/flashvm/swf_sound_mixer.h:54](../include/flashvm/swf_sound_mixer.h#L54) |
| `FVM_SWF_SOUND_MIXER_MAX_HORIZON` | macro / define | [include/flashvm/swf_sound_mixer.h:46](../include/flashvm/swf_sound_mixer.h#L46) |
<!-- source-links:end -->
