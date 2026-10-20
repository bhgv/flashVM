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
## Host transport and root loading since v0.674

[source.h](../frontend/common/source.h) defines FVM_DesktopSource (kind, bytes,
size, location), FVM_DesktopLoader (fetch, userdata, max_loads), FVM_DesktopWrite
and FVM_DesktopFetch. fvm_desktop_loader_init selects no transport and max_loads=1.
fvm_desktop_run_source accepts MEMORY, FILE or URL input with the same scalar
validation, ignored playlist, return/error and main-thread rules as memory run.
Non-memory sources must have null bytes and zero size. Names are 1..4096 bytes;
max_loads must be 1..1024 and includes startup. Source/policy/array storage remains
borrowed through synchronous return. FILE uses the SDL Unicode file adapter.
URL without a fetch callback fails explicitly; core VM acquires no network dependency.

Fetch receives a reference, optional previous canonical base, a write callback and
its userdata, a canonical-location output buffer/capacity, and host userdata.
Resolve relative references, deliver binary chunks synchronously, then write a
nonempty terminated effective absolute location within capacity. It becomes the
next movie's base. Do not retain any pointers, call VM/SDL, recurse playback or
throw C++ exceptions across the C boundary. Return the real transport status.
Stop on any write error. The sink bounds accumulation to 64 MiB, checks arithmetic,
latches the first error and discards partial/empty responses on failure, even if
a provider incorrectly ignores a write error. The host owns transport sessions,
authentication, cancellation and scheduling; this synchronous callback can block UI.

With max_loads>1 the runner installs the existing rooted AVM1 URL sink.
GetURL2 LOAD_TARGET requests to _root or _level0 with no method/GET copy the
counted reference into native storage. The old script may finish its current
callback/frame; no transport or recursive playback runs inside it. At the next
safe boundary the runner destroys the old VM/audio/window, fetches the next
movie, and starts a fresh VM. No managed pointer survives. One outstanding request
is accepted; a second is unsupported. The total load bound stops loops with
playback status 1; close stops the chain. A failed later load does not roll back
the already closed movie. Child/other-level targets, POST, loadVariables and browser
navigation are unsupported. This is bounded root replacement, not full Flash
loadMovie semantics or asynchronous progressive loading.

For FILE chains relative paths join the previous file directory; absolute paths
remain absolute. URL chains resolve through the provider. A memory movie may use
its location as the URL-provider base; absolute requests work independently of it.
Transport callbacks occur on the owner thread only after old VM cleanup.
[source_movie.c](../frontend/tests/source_movie.c) tests chunking, canonical bases,
partial failure, ignored overflow, malformed canonical output, root/level0 loads,
unsupported requests and loop limits in both interpreter and JIT.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_SWF_DEFAULT_LIMIT` | macro / define | [include/flashvm/swf.h:15](../include/flashvm/swf.h#L15) |
<!-- source-links:end -->
