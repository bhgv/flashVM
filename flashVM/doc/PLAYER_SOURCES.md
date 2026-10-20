# SWF sources and embedding

[Русский](PLAYER_SOURCES.ru.md).

Current v0.682 supports child movies and loadVariables; it supersedes the historical root-only restrictions below.

[Native POSIX/Winsock transport without libcurl](SOCKET_TRANSPORT.md) is available since v0.677.

## Memory input since v0.673

[fvm_desktop_run_memory](../frontend/common/player.h#L42) in [player.h](../frontend/common/player.h) accepts scalar
[FVM_DesktopOptions](../frontend/common/options.h#L29), a const void pointer, the exact size_t byte count, a nonempty
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

[source.h](../frontend/common/source.h) defines [FVM_DesktopSource](../frontend/common/source.h#L27) (kind, bytes,
size, location), [FVM_DesktopLoader](../frontend/common/source.h#L44) (fetch, userdata, max_loads), [FVM_DesktopWrite](../frontend/common/source.h#L35)
and [FVM_DesktopFetch](../frontend/common/source.h#L40). [fvm_desktop_loader_init](../frontend/common/source.h#L51) selects no transport and max_loads=1.
[fvm_desktop_run_source](../frontend/common/source.h#L60) accepts MEMORY, FILE or URL input with the same scalar
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
## HTTP(S), CLI and shared modules since v0.675

FVM_PLAYER_HTTP=ON adds the independent libcurl 7.62+ SDK dependency to the
optional frontend only. Default is OFF; custom transport and memory/file playback
remain available without it. [fvm_desktop_http_fetch](../frontend/common/http.h#L21) in [http.h](../frontend/common/http.h)
is an [FVM_DesktopFetch](../frontend/common/source.h#L40) provider: set loader.fetch to it and keep userdata null.
It resolves reference/base with libcurl's URL API, returns canonical provenance,
performs HTTP(S) GET with TLS peer/hostname verification, checks the entire transfer
and accepts only 2xx. Redirects are rejected, not followed; POST/authentication
configuration, progressive loading and cancellation are not implemented. Connection
and total timeouts are 10/30 seconds. OS trust-store and curl proxy configuration
apply. Calls own and balance a curl-global reference and private handles. An
embedding host using curl keeps its own global reference and coordinates global
initialization/cleanup, especially with older non-thread-safe SDKs.

The CLI and structured playlist detect a URI by ://. HTTP builds use this provider;
without a provider URI playback fails explicitly. --loads N sets a per-starting-
movie total root-load budget, including startup, from 1 to 1024 (default 1).
[FVM_DesktopOptions.max_loads](../frontend/common/options.h#L35) carries the same CLI/playlist policy; direct run_source
uses its explicit [FVM_DesktopLoader](../frontend/common/source.h#L44) policy. Closing a movie ends the entire playlist.
File playlists still default to one authored pass. Unsupported targets/POST/etc
propagate failure rather than being silently interpreted as root navigation.

```sh
cmake -S flashVM -B build-player -DFVM_BUILD_DESKTOP_FRONTEND=ON -DFVM_BUILD_EXAMPLES=ON -DFVM_PLAYER_HTTP=ON -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" -DSDL2_DIR=/path/to/sdl/lib/cmake/SDL2
cmake --build build-player --target flashvm_player
./build-player/frontend/flashvm_player --mute --loads 16 https://example.org/start.swf
```

For C/C++ embedding, include player.h and http.h, initialize options/loader, set
source.kind=FVM_DESKTOP_SOURCE_URL, bytes=null, size=0 and location to the URL;
set loader.fetch=[fvm_desktop_http_fetch](../frontend/common/http.h#L21) and call [fvm_desktop_run_source](../frontend/common/source.h#L60). For a
linked startup resource choose MEMORY and the exact array size; the same loader
can service absolute/relative requests made by the embedded movie. The provider
resolves relative references only when the supplied base is a valid HTTP(S) URL.

FVM_PLAYER_HOST_SHARED=ON changes flashvm_player_host into a shared module and
enables PIC for bundled static dependencies. OFF retains an archive. Windows
exports the host symbols through CMake; this remains an internal C89/C++98 frontend
interface. Use a target SDL/curl SDK, with PIC static dependencies when building
ELF/Mach-O shared libraries. Do not reuse non-PIC SDL archives. Bundled VM libraries
remain static; the option does not set global BUILD_SHARED_LIBS.

```sh
cmake -S flashVM -B build-module -DFVM_BUILD_DESKTOP_FRONTEND=ON -DFVM_BUILD_EXAMPLES=ON -DFVM_PLAYER_HOST_SHARED=ON -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" -DSDL2_DIR=/path/to/pic-sdl/lib/cmake/SDL2
cmake --build build-module --target flashvm_example_module_host
./build-module/frontend/flashvm_example_module_host
```

The [resource module example](../examples/player/embedded_movie/README.md) also
builds as an archive by default or shared module with that option. It contains
its own immutable SWF and exports embedded_movie_play for an ordinary C/C++ host;
no runtime movie file is required. Android retains SDLActivity; no desktop module
host is built. Native/cross SDK setup remains in the frontend guide.

The loopback HTTP regression tests startup and SWF-triggered relative GET, chunked
bodies, 404, rejected redirects, truncated transfers and local relative root loads
in interpreter/JIT. It does not certify public networking, trusted HTTPS endpoints,
physical devices or complete Flash loading semantics.
## Child clips, variables and working bases since v0.676

The FIFO accepts multiple GetURL2/global/MovieClip requests in one frame. The host
installs loadMovie and loadVariables global facades and MovieClip methods. GET or
no method is supported. Targets resolve through the active display scope, including
relative clip paths; _root follows native root/lockroot resolution, _level0 selects
the player root. loadVariables can target ordinary objects as well as clips.
Each queued request copies both its URL and its originating movie's source base,
and pins the resolved target until completion. No socket/file fetch runs inside
an active script callback. Child/variable operations run at the next owner-thread
safe point while the parent VM survives. Whole-root replacement still releases
the old VM; remaining requests belonging to it are discarded safely. Close or an
error releases every queued root. --loads and max_loads now count startup plus
all movie/form resource requests, not just root replacements.

A memory startup defaults to the executable directory, never process CWD.
Linux/Windows use SDL_GetBasePath; macOS uses dyld plus realpath (the executable,
not the app bundle Resources directory); Android reads /proc/self/exe. An APK
host can supply an explicit absolute source name for a different resource origin.
The macOS/Android discovery branches require actual target SDK/runtime acceptance. An absolute filesystem or HTTP(S) source name explicitly supplies provenance;
this is useful for a library host or an OS sandbox blocking executable discovery.
Without discovery or an explicit origin, startup still works but relative resources
fail rather than falling back to CWD. Tests isolate the OS discovery call because
this environment has no /proc/self/exe; they simulate the executable directory,
remove the original startup file, change CWD and exercise actual resource resolution.
A URL startup uses its canonical URL. Every loaded child retains its own canonical
location, and native descendants inherit the nearest source base. Thus a child's
loadVariables resolves from that child's directory, not the selected target or
startup directory. No process-wide chdir or network working-directory global exists.

### Core C89 APIs

[fvm_movieclip_set_source](../include/flashvm/movieclip.h#L103) copies a nonempty counted location (1..4096, no NUL) into
the private managed clip graph. Same-context clip and live owner thread required;
no script runs. [fvm_movieclip_get_source](../include/flashvm/movieclip.h#L107) reads the nearest native ancestor source,
returning a borrowed string; NOT_FOUND/errors preserve out. Retain the clip across
GC. Private source metadata travels with ordinary graph fields; host callback code
and transport sessions are not serialized.

[fvm_movieclip_replace_swf](../include/flashvm/movieclip.h#L114) imports into the same context without changing the target
object, parent depth slot, transform/shear or existing host/script own properties.
Only named bindings of old display children are removed; other saved references
survive. Old children are detached. The new library root points to the existing
target, and the new source is retained. No frame action runs until later advance.
All fallible staging precedes publication; malformed input/OOM preserves the old
content. Owner thread, outside VM/native callbacks/GC/teardown; nested replacement
returns INVALID_ARGUMENT. The strict current importer rejects unsupported tags and
child root streaming sound. Successful replacement requests a redraw.

[fvm_object_load_variables](../include/flashvm/movieclip.h#L122) takes a same-context target and counted form bytes up to
64 MiB. It decodes all percent escapes/plus/spaces and WTF-8 before writing, with
at most 65536 fields, including ignored empty segments. NUL in decoded values/keys remains counted.
Duplicate keys assign in input order, last value wins; missing equals means empty
value, empty keys are ignored. Malformed decoding changes no properties. Ordered
public setters may invoke script: failure retains already committed effects.
Optional onData is invoked after successful assignments; its failure also preserves
the data. Empty input completes successfully and invokes the optional handler.
The native collector barrier protects destination/temporary data across callbacks;
borrowed input lives through return. Success requests redraw. No transport is in
this core API, and no legacy non-UTF-8 code-page conversion is claimed.

The host facades support the common global and method argument shapes. AVM1 may
resolve an unqualified name through a clip prototype before global fallback; the
facade selects the shape from target versus GET/POST arguments. Full historical
lookup/shadowing/method-coercion conformance remains open. POST, browser navigation,
progressive/cancellable transport, full MovieClipLoader events and independent child sound dictionaries
(event/stream) remain unsupported. Available transports are optional libcurl, the native POSIX/Winsock HTTP(S) provider
and a host-supplied callback; see [native transport](SOCKET_TRANSPORT.md).

[Runnable loading example](../examples/player/loading/README.md) ·
[Core APIs](../include/flashvm/movieclip.h) ·
[Core implementation](../src/standard/movieclip_loading.cpp) ·
[Core tests](../tests/movieclip_loading.cpp) ·
[Allocation failure sweep](../tests/movieclip_loading_allocation.cpp) ·
[HTTP/embedded tests](../frontend/tests/child_loading.py)
### Opaque session cleanup and Windows exports

[fvm_desktop_shutdown](../frontend/common/player.h#L49) in player.h explicitly ends the matching library's SDL
session on its UI owner thread after playback returns. If SDL_WasInit reports any
active subsystem, it returns [FVM_ERROR](../include/flashvm/flashvm.h#L43) without touching it; otherwise it calls
SDL_Quit and returns [FVM_OK](../include/flashvm/flashvm.h#L42), including repeated shutdown of an unused session.
It can clear global hints/TLS, so the application calls it only when ending that
SDL session. No automatic global quit is added to window/VM cleanup.
This also handles a DLL containing a private static SDL instance: an ordinary
application need not link/call a second copy of SDL. The example module exports
embedded_movie_shutdown beside embedded_movie_play, and module_host.c includes
only module.h. Do not unload any module while a run is active.

Windows DLL declarations use FVM_DESKTOP_API in export.h with explicit dllexport/
dllimport. CMake supplies FVM_DESKTOP_SHARED publicly and FVM_DESKTOP_BUILD privately
for shared hosts. A manual consumer of a player-host import library defines
FVM_DESKTOP_SHARED; a static archive consumer does not. ELF/Mach-O keep default
visibility. Linux player RPATH searches beside the executable and in its lib/
subdirectory. Shared native CTest adds the build-library directory explicitly
where sandbox /proc restrictions prevent $ORIGIN discovery. Busy-owner shutdown
is tested against a shared SDL instance in the static-host fixture; opaque DLL
cleanup is tested through the module interface.
GET here means retrieving the supplied URL. Automatic serialization of current-clip variables into query parameters or a POST body is not implemented.
[Controlled/background C APIs / Управляемая фоновая загрузка](BACKGROUND_TRANSFER.md), v0.680–0.682.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_movieclip_get_source` | function | [include/flashvm/movieclip.h:107](../include/flashvm/movieclip.h#L107) |
| `fvm_movieclip_replace_swf` | function | [include/flashvm/movieclip.h:114](../include/flashvm/movieclip.h#L114) |
| `fvm_movieclip_set_source` | function | [include/flashvm/movieclip.h:103](../include/flashvm/movieclip.h#L103) |
| `fvm_object_load_variables` | function | [include/flashvm/movieclip.h:122](../include/flashvm/movieclip.h#L122) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_SWF_DEFAULT_LIMIT` | macro / define | [include/flashvm/swf.h:15](../include/flashvm/swf.h#L15) |
<!-- source-links:end -->
