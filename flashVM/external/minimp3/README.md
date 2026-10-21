# Pinned minimp3 / Зафиксированный minimp3

Upstream: https://github.com/lieff/minimp3 at
`ea99364f61c14656440e8d77e9c233ccf3124633`. License: CC0-1.0; the original
[notice](upstream/LICENSE) and header are unchanged. PROVENANCE.json records the
pin, and ../SOURCES.sha256 verifies both upstream files. flashvm.patch is empty:
there are no local upstream changes.

[decoder.cpp](decoder.cpp) compiles the scalar Layer-III-only configuration in
strict C++98 and prefixes its two external symbols. No file/device/heap helpers,
minimp3_ex, media SDK, runtime downloads or ISA-specific instructions are required.
The upstream private header uses compiler-provided stdint.h; it is not a new
public C89 host dependency. CMake builds fvm_minimp3 STATIC and links it into the
core, including NO_MEDIA builds. Keep host-specific output in ports/.

Зафиксированы исходные header/LICENSE без изменений. Wrapper включает только
scalar Layer III, переименовывает внешние symbols и не открывает файлы/устройства.
Static core decoder не зависит от FFmpeg или установленного MP3 SDK.
[Integration and validation / Интеграция и проверки](../../doc/MP3_AUDIO.md).
