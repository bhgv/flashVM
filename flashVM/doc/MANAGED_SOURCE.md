# Managed movie acquisition — v0.1703

[Русский](MANAGED_SOURCE.ru.md) · [Accepted DisVM modes](../../disVM/runtime/FLASHVM-IO-MODES.md)

The shared player now has two explicit source entry points. Ordinary
[`fvm_desktop_run_source`](../frontend/common/source.h#L75) retains its current native
file/optional HTTP behavior. New [`fvm_desktop_run_source_managed`](../frontend/common/managed_source.h#L48)
delegates **every supported file/URL acquisition** to an explicitly supplied
native bridge, including startup, root replacement, child movie loads and
`loadVariables`. The bridge can bind the existing DisVM terminal connection.

This release implements the player side of managed acquisition. It does not
yet encode that bridge's requests as Styx/S-record messages or implement its
DisVM server. It adds no socket/file-writing API, media stream transport or
input filtering. Physical platform and full SWF compatibility stages remain open.

## API and ownership

| API/type | Contract |
| --- | --- |
| [`FVM_DesktopManagedLoader`](../frontend/common/managed_source.h#L28) | Caller descriptor with exact `struct_size`, required `fetch`, borrowed `userdata`, and existing total `max_loads` budget |
| `fvm_desktop_managed_loader_init(out)` | Return1 and initialize descriptor, no bridge and startup-only budget1; NULL returns0; no allocation/I/O |
| [`FVM_DesktopManagedFetch`](../frontend/common/managed_source.h#L21) | Synchronous native callback receiving purpose, raw reference/base, byte sink, resolved-location output/capacity and host state |
| `fvm_desktop_run_source_managed(options, source, loader, error)` | Snapshot policy and run on the player owner thread; desktop0 success/stopped,1 acquisition/playback failure,2 argument failure; optional static error |

The caller keeps options, source bytes and transport state valid through return.
No concurrent/reentrant player runs are added. The callback may block like the
legacy synchronous source transport; an asynchronous managed runner is separate
integration work. The callback must not enter VM/SDL, destroy the player or
retain its borrowed inputs/sink. The native sink/userdata pointers are local ABI
objects and must never be serialized onto the connection.

The bridge resolves the raw reference against the supplied movie base **inside
the delegated VFS** and feeds arbitrary binary chunks through the sink. It must
stop on a non-OK sink status, return the actual acquisition result, and supply a
nonempty bounded NUL-terminated canonical VFS location. The existing movie-size
limit applies. Any transport/sink/origin failure discards staged bytes before
import; the player never retries that failure through a local file or HTTP loader.

## Purpose, provenance and policy

| Purpose | Operation | DisVM adapter mapping |
| --- | --- | --- |
| [`FVM_DESKTOP_FETCH_CODE`](../frontend/common/managed_source.h#L8) | Startup/root/child execution-capable movie acquisition | Provider-first [`DVM_RESOURCE_SWF`](../../disVM/runtime/vfs/intent.h) |
| [`FVM_DESKTOP_FETCH_DATA`](../frontend/common/managed_source.h#L9) | Variable/form resource acquisition | Ordinary [`DVM_RESOURCE_DATA`](../../disVM/runtime/vfs/intent.h) |

These constants are native purpose identifiers, not assigned wire tags or new
permissions. A MovieClip movie-load operation may accept a supported image as
well as SWF; it remains execution-capable at request time and uses CODE purpose.
`loadVariables` always uses DATA, even when its name ends in `.swf`. Filename
suffixes do not decide policy.

Managed FILE and URL descriptors both invoke the bridge. A MEMORY startup movie
has already been supplied by the bridge owner: its location is preserved as the
explicit VFS provenance, including a relative origin. There is no executable-path
or process-CWD substitution. Each imported child keeps its own returned canonical
base for later requests.

Access policy belongs to the bridge/DisVM VFS. Managed acquisition invokes neither
the native file reader nor the compiled HTTP policy adapter. AVM1 request-shape
validation, importer limits and the existing GET/no-method support remain in
force; unsupported POST/browser-navigation operations still fail explicitly.
Other embeddings and arbitrary trusted host extensions must install their own
corresponding managed route; this is not a process-wide OS sandbox switch.

The existing S-record framing, channel ownership and future extra-channel policy
remain as specified by the accepted DisVM contract. This API deliberately keeps
FlashVM detachable: no DisVM headers, guest pointers or sibling build dependency
are introduced into the player.

## Validation

The actual source-player test runs interpreter and JIT against independent raw
SWFs. A startup movie creates a child, loads its SWF, reads root variables and
executes a child-relative variable load; script checks the decoded values. Three-
byte bridge chunks verify that resource reads do not assume record boundaries.
The same execution succeeds from FILE, URL and already supplied MEMORY sources.
Deferred root replacement retains the bridge across VM teardown, updates the
canonical base for subsequent child/data loads and observes the shared budget.

A valid native `.data` movie file exists during failure tests. Bridge NOT_FOUND,
partial bytes followed by error, ignored sink overflow and unterminated provenance
all fail with one bridge call; that local movie is never used as fallback. Missing
bridge and invalid descriptor refuse before acquisition. The fixture also checks
CODE on a `.data` startup and DATA on a `.swf` variable resource.

The legacy player implementation was split into bounded owner-local composition
units; before functional changes its preprocessed native tokens matched the old
implementation exactly. This is a source-layout change, without new worker/GC
ownership semantics.

```sh
cmake -S flashVM -B build/player \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" \
  -DFVM_BUILD_DESKTOP_FRONTEND=ON -DFVM_BUILD_TESTS=ON -DFVM_BUILD_EXAMPLES=OFF
cmake --build build/player --target flashvm_frontend_source_movie --parallel
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software SDL_AUDIODRIVER=dummy \
  ctest --test-dir build/player -R '^flashvm_player_source_' --output-on-failure
```

Requires the SDL2 development SDK and a C89/C++98 toolchain. SDL dummy-video
acceptance is separate from physical display/input-device acceptance.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_DESKTOP_FETCH_CODE` | macro / define | [frontend/common/managed_source.h:8](../frontend/common/managed_source.h#L8) |
| `FVM_DESKTOP_FETCH_DATA` | macro / define | [frontend/common/managed_source.h:9](../frontend/common/managed_source.h#L9) |
| `fvm_desktop_managed_loader_init` | function | [frontend/common/managed_source.h:39](../frontend/common/managed_source.h#L39) |
| `fvm_desktop_run_source` | function | [frontend/common/source.h:75](../frontend/common/source.h#L75) |
| `fvm_desktop_run_source_managed` | function | [frontend/common/managed_source.h:48](../frontend/common/managed_source.h#L48) |
| `FVM_DesktopManagedFetch` | callback typedef | [frontend/common/managed_source.h:21](../frontend/common/managed_source.h#L21) |
| `FVM_DesktopManagedLoader` | struct | [frontend/common/managed_source.h:28](../frontend/common/managed_source.h#L28) |
<!-- source-links:end -->
