# SWF sources and embedding

[Русский](PLAYER_SOURCES.ru.md).

## Memory input since v0.673

fvm_desktop_run_memory in [player.h](../frontend/common/player.h) accepts scalar
FVM_DesktopOptions, a const void pointer, the exact size_t byte count, a nonempty
source name and optional static error output. char buf[] and read-only linked
arrays both work; binary NUL is ordinary data. No temporary file, strlen(data),
source mutation or ownership transfer occurs. Keep source bytes and name immutable
and accessible until synchronous return. VM import owns its copied representation.
Both FWS and CWS use the same importer, preflight, scene, GC roots, scheduler,
rendering and cleanup as file playback.

The playlist fields are ignored; every scalar setting is validated. Null data,
zero/over-limit length, missing name or malformed options returns argument status 2
without creating a VM/window. Invalid SWF returns playback status 1 before window
creation. Success/close returns 0. Optional error receives null or a static message;
never free it. Other diagnostics retain the existing standard streams. Input is
limited to [FVM_SWF_DEFAULT_LIMIT](../include/flashvm/swf.h#L15) (64 MiB), and decoded limits still apply.

Use the native UI/VM owner thread, without concurrent or recursive playback.
This is synchronous playback, not a widget API. A resource name is a label and
provenance, not a file opened by memory playback. Final global SDL shutdown belongs
to the application. [Ownership](PLAYER_HOST_OWNERSHIP.md).

[Embedded executable example](../examples/player/embedded_movie/README.md) links
an immutable SWF into the executable and needs no asset file at runtime.
C89 regressions play stack char input and compressed input in interpreter/JIT,
reject invalid descriptors, check unchanged bytes and use nonexistent source paths.

```sh
cmake --build build-player --target flashvm_example_embedded_movie flashvm_frontend_memory_movie
./build-player/frontend/flashvm_example_embedded_movie
ctest --test-dir build-player -R 'player_memory|example_embedded' --output-on-failure
```

[Build/native/cross guide](DESKTOP_FRONTEND.md) ·
[Structured host](PLAYER_CONFIGURATION.md) · [Implementation](../frontend/common/player.c) ·
[Roadmap status](ROADMAP_STATUS.md)

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_SWF_DEFAULT_LIMIT` | macro / define | [include/flashvm/swf.h:15](../include/flashvm/swf.h#L15) |
<!-- source-links:end -->
