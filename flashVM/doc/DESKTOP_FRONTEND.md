# Desktop SWF application — v0.577

v0.582 update: [test builds, HostInput, event audio and Android APK](PLAYER_TEST_BUILDS.md).


[Русская версия](DESKTOP_FRONTEND.ru.md).

## Build and launch

The application lives entirely in [frontend/](../frontend/README.md), with Linux,
Windows and macOS entrypoints and shared C89 code. It is built inside the VM
component and is optional; compiler/linker projects have no SDL dependency.
Install a native C89/C++98 toolchain, CMake and an SDL2 2.0.9+ development package
providing its CMake config/`SDL2::SDL2` target. Then, from `flashVM`:

```sh
cmake -S . -B build-desktop -DFVM_BUILD_DESKTOP_FRONTEND=ON
cmake --build build-desktop --target flashvm_player --parallel
./build-desktop/frontend/flashvm_player --help
./build-desktop/frontend/flashvm_player --jit --quality best --speed 150% movie.swf
./build-desktop/frontend/flashvm_player --no-jit --fps 24 first.swf second.swf
```

The selected DEFS profile must enable GRAPHICS_AGG or GRAPHICS_CAIRO. The core's
HEADLESS output choice still means a host-owned RGBA buffer; this optional host
presents it through SDL2 instead of changing the VM's rendering backend. The VM's
bundled libraries remain static; SDL2 is an external host SDK with its own runtime
and notices. Use `SDL2_DIR`/`CMAKE_PREFIX_PATH` for a nonstandard SDK. Do not assume
`origin` identifies the GitHub repository when transporting this change.

OS preparation: [Linux](../frontend/linux/README.md),
[Windows](../frontend/windows/README.md), [macOS](../frontend/macos/README.md).
[Android](../frontend/android/README.md) has a separate SDLActivity/Surface APK target since v0.582.

## Complete command line

Values are separate arguments, not `--option=value`. Options may precede or
follow filenames until `--`. Quote filenames containing spaces. `--` permits
paths that begin with a dash. Input paths are local files, not URLs.

| Option | Behavior |
| --- | --- |
| `-h`, `--help` | Prints help successfully without opening a file, display or VM |
| `--version` | Version, ABI and build metadata without opening files or devices |
| `--jit` | Requests automatic JIT; unavailable backend is an error before window creation. Unsupported bodies may still use the interpreter |
| `--no-jit`, `--interpreter` | Forces interpreter mode and bypasses automatic/attached JIT |
| `--quality low\|medium\|high\|best` | Selects starting stage quality; default high. SWF can later change it |
| `--fps N` | Absolute tick rate, decimal 0.1..1000; independent of authored FPS |
| `--speed N` or `--speed N%` | Decimal 0.1..10000 percent of authored FPS; default 100 |
| `--once` | Runs the authored frame count worth of ticks, then closes the current movie |
| `--frames N` | Positive integer per-file tick limit; useful for bounded launches/tests |
| `--mute` | Mutes event audio and validated root streams; skips device opening |
| `--stats` | Reports tick count and actual native/interpreter/fallback counters |

Conflicting interpreter/JIT options, `--fps` with `--speed`, and `--frames` with
`--once` are errors (status 2). Repeated values of the same option use the last
value. Unknown options, missing values, NaN/Infinity, overflow, signed/zero frame
limits, numeric whitespace and invalid suffixes are rejected. A percent suffix
is accepted only for --speed. Exit status 0 means help/normal finish/close;
1 means loading, VM or window failure; 2 means malformed command line.

## Timing and playlist

Without an explicit execution mode, the host selects automatic JIT when available
and interpreter otherwise. `--stats` is the evidence for actual native entries;
the requested mode alone is not evidence that all code ran natively.

Authored FPS = `frame_rate_fixed8 / 256`. Effective FPS is either `--fps` or
`authored FPS * percent / 100`, never both. Effective rate must be positive and at
most 1000. Very slow positive authored/scaled rates remain valid. A zero-rate
SWF needs an explicit --fps override. Each scheduled tick calls
`fvm_movieclip_advance(context, root, 1)`, clears the stage, renders and presents.
The first tick enters frame 1. The final bounded frame remains visible for its
scheduled period. Expose/resize repaints the retained texture without advancing
the VM, so a slow rate does not prevent window refresh or close handling.

SDL performance counters provide monotonic deadlines; the event loop waits
rather than busy-spinning. Excessive lag resets a deadline without skipping VM
ticks. CPU/display load can reduce achieved speed; no hard real-time guarantee
is made. This controls timeline ticks, not a virtualized GetTime clock, audio
resampling, video decode or every timer in the Flash API.

A single file repeats its timeline under script control until Escape/window close
unless bounded. A playlist runs files in order with fresh VM/window owners and a
default limit of each file's declared frame count. Explicit --frames overrides
that limit. A script stop/goto may cause repeated/skipped authored frame numbers;
the limit counts host ticks, not unique visited frames. Escape/window close ends
the entire playlist. Any file failure stops the playlist with an error.

## Presentation and ownership

The application reads a seekable file of at most [`FVM_SWF_DEFAULT_LIMIT`](../include/flashvm/swf.h#L15) (64 MiB).
Canonical loading supports the existing FWS/CWS profile. The scene importer can
reject unsupported tags; the frontend reports the VM status instead of ignoring
them. Stage dimensions come from the authored RECT, not currently visible shape
bounds. Twips use [`FVM_SWF_TWIPS_PER_PIXEL`](../include/flashvm/swf.h#L27) (20). Pixel dimensions are rounded up
and must fit 1..4096 per axis. The host viewport subtracts the authored left/top
origin without changing `_x`, `_y`, visibility, color or child transforms.

The host owns the RGBA surface, native window, renderer, texture, context and
runtime. The root clip is explicitly pinned before script execution; GC stepping
occurs only after complete render/present calls. Cleanup releases SDL objects,
pixel/file buffers, the root pin and VM owners on success and failure. No VM
pointer is retained by the window adapter. Every VM/event/render call stays on
the main thread; there is no cross-thread VM access.

Each tick clears pixels to selected stage background or opaque white. SDL receives
RGBA32 bytes, ignores stage alpha for ordinary OS-window presentation, and keeps
aspect ratio/letterbox margins during resize. Source pixels are borrowed only for
upload. The quality switch uses the existing [stage policy](STAGE_QUALITY.md):
LOW disables vector edge AA; full distinctions among medium/high/best, bitmap
sampling and filter compatibility remain roadmap work.

## New public VM API

### fvm_swf_stage_bounds

Declared with [`FVM_SWFStageBounds`](../include/flashvm/swf.h#L29) in [swf.h](../include/flashvm/swf.h).
Pass an owning-thread loaded context and nonnull output with exact `struct_size`.
OK copies signed `x_min/x_max/y_min/y_max` in twips. Equal edges are valid metadata;
inverted edges are INVALID_ARGUMENT. Null/wrong-sized output, terminal/collecting
context or invalid canonical RECT also return INVALID_ARGUMENT. Unloaded context
returns UNSUPPORTED. Every failure preserves the complete output. No allocations,
callbacks, script, GC, borrowed bytes or retained ownership are involved.

### fvm_movieclip_render_view

Declared in [movieclip.h](../include/flashvm/movieclip.h). Arguments are owning
context, same-context MovieClip, valid caller-owned RGBA surface and nonnull
[`FVM_Affine`](../include/flashvm/path.h#L33) viewport. All six viewport values must be finite; singular matrices
are not rejected just for being singular. The matrix is copied before getters.
It is multiplied on the left of each authored world matrix exactly once, including
children, while preserving ordinary root visibility/color and script transforms.

Returns OK, INVALID_ARGUMENT for null/foreign/terminal inputs or nonfinite view,
OUT_OF_MEMORY on native allocation failure, and existing render/getter/script
statuses including UNSUPPORTED/EXCEPTION. Invalid viewport checks precede drawing;
other later render failures can leave a partially drawn destination, as with
ordinary rendering. It is not transactional. Caller must root the clip, retain
the surface for the call and avoid overlapping viewport/source/output storage.
No viewport pointer is retained. Native guard prevents collection/release during
render callbacks; invoke only on the owning VM thread outside terminal GC phases.

## Source map and internal functions

| Source | Functions and role |
| --- | --- |
| [options.c](../frontend/common/options.c) | [`decimal`](../frontend/common/options.c#L11), [`frame_limit`](../frontend/common/options.c#L88): checked numeric syntax; [`fvm_desktop_parse`](../frontend/common/options.h#L58): atomic option publication with caller-owned file scratch array; [`fvm_desktop_rate`](../frontend/common/options.h#L61): checked timing; [`fvm_desktop_help`](../frontend/common/options.h#L64): complete help |
| [player.c](../frontend/common/player.c) | [`read_movie`](../frontend/common/player.c#L60): bounded UTF-8 file adapter; [`checked`](../frontend/common/player.c#L111): operation/status diagnostics; [`clear_stage`](../frontend/common/player.c#L120): background initialization; [`play`](../frontend/common/player.c#L211): VM/window lifecycle and deadlines; [`fvm_desktop_main`](../frontend/common/player.h#L57): parser and playlist entry |
| [sdl_window.c](../frontend/common/sdl_window.c) | [`fvm_desktop_window_open`](../frontend/common/window.h#L53) / [`fvm_desktop_window_present`](../frontend/common/window.h#L56) / [`fvm_desktop_window_events`](../frontend/common/window.h#L59) / [`fvm_desktop_window_close`](../frontend/common/window.h#L88): native resource boundary; [`draw_texture`](../frontend/common/sdl_window.c#L109)/[`handle_event`](../frontend/common/sdl_window.c#L331): retained repaint and close processing; [`fvm_desktop_now`](../frontend/common/window.h#L83): monotonic time; [`fvm_desktop_window_error`](../frontend/common/window.h#L85): borrowed diagnostic |
| [options.h](../frontend/common/options.h), [window.h](../frontend/common/window.h) | Internal structures, timing/format constants and ownership contracts; not a stable public VM ABI |

## Checks and remaining work

Linux checks use actual SDL2/X11 software windows under Xvfb, FWS/CWS fixtures,
negative stage origin, Unicode/space filenames, both timing policies, playlist,
actual interpreter/JIT counters, pixel readback and close events. Core tests check
signed RECT widths, output preservation and viewport projection/visibility without
script geometry mutation. Help and invalid CLI tests use a nonexistent video
driver to prove no display initialization is required.

Enable native-display tests explicitly with `-DFVM_DESKTOP_WINDOW_TESTS=ON`, then
run CTest in a display session. They are not registered for cross builds. This
checkpoint does not certify native Windows/macOS, Wayland, a physical GPU or
Android. v0.582 adds explicit root event audio and a HostInput extension; full
A/V clock synchronization, standard Flash keyboard/button routing, browser plugin
and desktop file association remain open. Unsupported SWF
features remain errors; this is a desktop host for the implemented VM subset,
not a claim of full Flash Player compatibility or completion of roadmap stage 9.
Linux checkpoint v0.577: the desktop-enabled profile passes 1140 tests with
3 explicit foreign-native skips out of 1143 registered (examples disabled).
Four Cairo core rendering/RECT tests and eight viewer tests also pass.


The current bounded Key/Mouse listener routing and primary touch adapter are
described in [player input](PLAYER_POINTER.md). Full scene/button routing remains open.
[Current build metadata](PLAYER_BUILD_INFO.md) · [GC and C host arguments](PLAYER_EVENT_GC.md).
[Structured C89/C++98 host / Встраивание через структуру](PLAYER_CONFIGURATION.md).
[Memory, HTTP(S), SWF root loading and library modules / Источники и модули](PLAYER_SOURCES.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Affine` | struct | [include/flashvm/path.h:33](../include/flashvm/path.h#L33) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:66](../include/flashvm/movieclip.h#L66) |
| `fvm_movieclip_render_view` | function | [include/flashvm/movieclip.h:79](../include/flashvm/movieclip.h#L79) |
| `FVM_SWF_DEFAULT_LIMIT` | macro / define | [include/flashvm/swf.h:15](../include/flashvm/swf.h#L15) |
| `fvm_swf_stage_bounds` | function | [include/flashvm/swf.h:38](../include/flashvm/swf.h#L38) |
| `FVM_SWF_TWIPS_PER_PIXEL` | macro / define | [include/flashvm/swf.h:27](../include/flashvm/swf.h#L27) |
| `FVM_SWFStageBounds` | struct | [include/flashvm/swf.h:29](../include/flashvm/swf.h#L29) |
<!-- source-links:end -->
