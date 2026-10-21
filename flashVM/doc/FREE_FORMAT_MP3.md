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

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_mp3_probe` | function | [src/swf/sound_mp3.cpp:52](../src/swf/sound_mp3.cpp#L52) |
<!-- source-links:end -->
