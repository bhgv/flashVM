# Player presentation recovery

[Русский](PLAYER_RECOVERY.ru.md). The optional SDL player keeps one owned packed
RGBA copy of the last successfully uploaded frame. The source surface is borrowed
only during [`fvm_desktop_window_present`](../frontend/common/window.h#L54); it may be reused or freed immediately
on return. Row padding is excluded from the retained copy. No VM value or GC root
is stored in the window adapter. Closing the window frees the snapshot before
SDL shutdown. Allocation failures are reported, rather than retaining caller data.

The snapshot has the fixed logical stage size (four bytes per pixel), adding up
to 64 MiB at the existing 4096×4096 player limit. This is in addition to the host
raster surface and SDL texture. It is native presentation storage, not a SWF cache.
A rejected extent or insufficient input byte count leaves the retained image
unchanged. A later accepted presentation replaces it.

`flashvm_frontend_retained_frame` uses an actual SDL dummy software renderer and
checks padded rows, caller-buffer reuse, rejected surfaces and replacement.
This is Linux host protocol/renderer validation; it does not certify visible
windows or Android/Windows/macOS device recovery.

## Device and render-target reset (v0.648)

SDL_RENDER_DEVICE_RESET recreates the streaming texture, sets its blend mode,
uploads the retained frame and redraws it on the main thread. SDL_RENDER_TARGETS_RESET
only redraws: the player uses a streaming texture, not a target texture. A reset
before the first frame creates empty storage without presenting undefined pixels.
No timeline advance, script/timer callback or additional VM rendering occurs.
Exposure/resize also redraw the uploaded texture.

[`fvm_desktop_window_status`](../frontend/common/window.h#L61) reports persistent presentation/input failures
separately from the normal close result of [`fvm_desktop_window_events`](../frontend/common/window.h#L56). The player
checks it in active, background and final-frame waits; failed restoration is not
reported as successful user cancellation. Repaint errors are latched as well.
The SDL test injects both reset events and reads actual restored texture pixels
through the software renderer before presentation. It does not reset a GPU.

## Background input isolation (v0.649)

Both WILLENTERBACKGROUND and DIDENTERBACKGROUND clear held keys/buttons, primary
touch and the pending transition FIFO. Key/mouse/finger events while backgrounded
are discarded, including stale Escape/Back. WILLENTERFOREGROUND does not resume;
DIDENTERFOREGROUND does. Quit, owned-window close and graphics reset still work
while suspended. Desktop focus loss retains its previous clear-input policy and
does not pause playback.

Window, keyboard and mouse events carrying a nonzero foreign window ID are
ignored, including foreign Escape and close. ID zero remains accepted for
explicit synthetic/unassigned events. Touch keeps its selected-contact routing;
this change does not claim a complete multi-window SDL host. The lifecycle test
uses the real SDL queue for application events and injected native transitions,
not a physical Android Activity.

## Deferred foreground recovery (v0.650)

Device resets during a background interval only mark texture storage invalid.
They never allocate/upload/draw while suspended. DIDENTERFOREGROUND performs one
recreation for any number of intervening device resets, then displays the last
frame immediately, even if the next authored frame is seconds away. A foreground
notification also redraws a valid retained texture. WILLENTERFOREGROUND alone
keeps suspension. Recovery before the first frame does not draw uninitialized
pixels. The fixture counts actual texture creation calls to distinguish deferred
recovery from redundant recreations. Native GPU/surface loss remains untested.

## Final-frame audio policy (v0.651)

The final authored-frame hold now applies the same foreground check as the active
loop. While suspended it explicitly pauses an existing native output; it never
calls the tick operation that would unpause the worker. Foreground ticks resume
normal priming. The hold deadline is still wall-clock based; suspension does not
extend a completed movie or keep a playlist entry alive indefinitely.

`flashvm_capture_tail_pause_interpreter/jit` runs the real player, AVM1 and AGG
rasterizer with a deterministic protocol window/clock and an observed audio
adapter. Its background interval lies entirely inside the final-frame hold.
A background tick fails immediately; assertions also require pause calls,
foreground resume, one timeline tick and one raster presentation. The adapter
opens no device, so this is scheduling-policy evidence, not audible playback.

## Build and reproduce

From the repository root, supply your installed SDL2 SDK explicitly if CMake
cannot find it (SDL2_DIR points to the directory containing SDL2Config.cmake):

```sh
cmake -S flashVM -B build/player \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" \
  -DFVM_BUILD_EXAMPLES=OFF -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_ENABLE_RENDER=ON -DSDL2_DIR=/absolute/path/to/SDL2/cmake
cmake --build build/player --target flashvm_player \
  flashvm_frontend_retained_frame flashvm_frontend_lifecycle \
  flashvm_frontend_tail_capture
ctest --test-dir build/player \
  -R '^flashvm_(frontend_(retained_frame|lifecycle)|capture_tail_pause_)' \
  --output-on-failure
```

The JIT fixture needs an available native JIT backend; use the interpreter test
on a target without one. The protocol tail fixture is native-only. Cross builds
use the [explicit SDK guide](../../BUILDING.md) and a **target** SDL2 library,
never a host SDL library. Android keeps its NDK/SDLActivity workflow; macOS uses
its platform SDK. No new Android/macOS SDK build or foreign runtime acceptance
is claimed here.

For native manual acceptance, play a slow movie and repeatedly resize/expose the
window, then background/restore the application where its OS supports SDL app
lifecycle notifications. Check that the old image returns before the next frame,
background taps do not fire after resume, and a bounded final frame does not
restart audio in the background. Exercise actual GPU/surface loss on that target;
sending an SDL event alone is insufficient evidence of device recovery. Keep the
OS/CPU, SDK/video/audio drivers and movie identity with the result. All nine
roadmap stages remain open; this series addresses bounded player lifecycle gaps.

## Validation of this series

Linux x86-64, GCC/G++ 13.3, CMake 4.4.4: 31 selected player/frontend tests pass
with the AGG/JIT profile. This includes existing timer, ClipActions, keyboard,
pointer, updateAfterEvent and interpreter/native-JIT movies, plus retained-frame,
lifecycle and final-hold tests. The old final-hold implementation fails the new
protocol regression as intended. No full core suite or fresh sanitizer run is
claimed. The fixture was updated to resume foreground explicitly before testing
normal key delivery; background key delivery is now deliberately forbidden.

The external SDL2 SDK is release-2.30.12, commit
8236e01a9f758d15927624925c6043f84d8a261f. It is built static outside this repository.
Linux SDK checks disable X11, Wayland, ALSA, PulseAudio, PipeWire, DBus and IBus;
dummy video is explicit. Those SDKs do not certify a desktop display/audio setup.
Cross SDKs use the existing Zig 0.13 host-wrapper adaptation described in
[toolchain provenance](../../toolchains/VALIDATION.md); this is not a claim that
an unmodified stock Zig package was validated. The player uses its full AGG/JIT
profile and a separately built **target** SDL SDK. Android/macOS acceptance,
physical devices, endurance and all nine roadmap stages remain open.

Full player/AGG/JIT plus target SDL cross-builds link for Linux ARM64, Linux
RISC-V64 and Windows x64. ELF/PE machine headers were checked; no foreign
executable was run. SDL audio output is not enabled in these build checks.
