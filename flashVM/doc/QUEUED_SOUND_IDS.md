# Queued sound identities

## v0.793: prepared duration on the output grid

[`fvm_swf_sound_voice_output_frames`](../include/flashvm/swf_sound_mixer.h#L44) counts exact remaining output frames for a
prepared native voice adopted at its CURRENT source cursor with conversion phase
zero. It includes remaining repetitions, rounds up the rational source/output
ratio and returns zero at EOF. It does not read PCM, allocate, call a callback,
access a VM or advance the source. Output rate must be within the configured
mixer range; bad arguments return INVALID_ARGUMENT and preserve output. A
count beyond SOUND_OUTPUT_MAX_CLOCK is UNSUPPORTED. Caller retains and serializes
the voice. The result is an integer-valued double, not seconds or a DAC position.
An already adopted mixer voice has its own lookahead/phase and is not described
by this fresh-grid calculation.

`swf_sound_voice_output_frames` compares every cursor in a three-repeat interval
at four source and eight output rates with actual one-frame mixer retirement,
including EOF and post-VM lifetime. This is native sample accounting, not full
Flash timeline or physical-device acceptance.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_sound_voice_output_frames` | function | [include/flashvm/swf_sound_mixer.h:44](../include/flashvm/swf_sound_mixer.h#L44) |
<!-- source-links:end -->
