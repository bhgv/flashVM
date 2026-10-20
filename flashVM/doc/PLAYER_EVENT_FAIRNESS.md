# Bounded SDL event processing

[Русский](PLAYER_EVENT_FAIRNESS.ru.md). Since v0.658 each call to
[`fvm_desktop_window_events`](../frontend/common/window.h#L58) handles at most 64 SDL records. On reaching that limit
it returns immediately without waiting, leaving the remaining queue intact.
The player can then deliver input, poll timers, take a safe GC step and advance
or draw the movie. Window/exposure/user event storms cannot trap the host in one
unbounded SDL drain. A close behind a large backlog is delayed by bounded batches,
never discarded. Calls that drain fewer records retain the requested wait policy.

This budget is separate from the 128-entry copied input FIFO. It does not promise
bounded total SDL queue memory, event coalescing or lossless hostile producers.
FIFO overflow still fails explicitly. All SDL and VM operations remain on the
owner thread; this adds no VM worker or concurrent callback execution.

flashvm_frontend_event_budget uses an actual dummy SDL queue, verifies remaining
records after each batch and proves that queued keyboard/quit records survive.
The deliberately long wait argument must not block while a full batch was used.
This is a scheduling/queue contract check, not OS/device endurance acceptance.

## Touch window identity (v0.659)

SDL 2.0.12+ identifies a touch event's destination window. The same owned-window
filter now covers down/motion/up finger records, including a foreign release
with matching touch/finger IDs: it cannot terminate our contact. Zero IDs remain
explicit synthetic/unassigned input; older SDKs retain their device/finger-only
routing. Primary-contact selection and focus/background cancellation are unchanged.
The lifecycle fixture checks foreign start/release suppression and owned release.

## Cursor ownership boundary (v0.660)

The movie's cursor request is now applied only while the pointer belongs to its
foreground window. Leave/focus loss/background immediately restore the visibility
captured when the host opened; they do not change the VM's requested value.
Re-entering allows the next normal owner-thread sync to apply that request again.
Foreground/focus regain refresh ownership from SDL_GetMouseFocus. This avoids
leaving the SDL-global cursor hidden over another window after Mouse.hide.
A missing physical cursor is still not an error. The cursor fixture checks real
SDL visibility across enter/leave/background and unchanged context requests.

## Reproduce and validation (v0.661)

Use the [AGG/JIT + SDL build recipe](PLAYER_RECOVERY.md#build-and-reproduce), then:

```sh
cmake --build build/player --target flashvm_player flashvm_frontend_clip_player \
  flashvm_frontend_event_budget flashvm_frontend_wheel flashvm_frontend_lifecycle \
  flashvm_frontend_cursor
ctest --test-dir build/player \
  -R '^flashvm_(frontend_(event_budget|wheel|lifecycle|cursor)|player_(wheel|event_storm)_)' \
  --output-on-failure
```

The storm movie fixtures insert three full batches ahead of motion/wheel events,
then execute actual SWF6–8 callbacks in interpreter and native JIT. Good and
wrong-value movies verify ordered delivery survives the backlog. Test-only cursor
observations wait for both the SDL queue and copied FIFO to deliver their input;
no production event injection was added. These tests require no physical display.

Linux x86-64/GCC 13.3, external SDL2 2.30.12: the selected frontend suite contains
38 tests. The corrected queued-wheel test also rejects the previous v0.656
implementation. The full AGG/JIT player links with separate target SDL SDKs for
Linux ARM64, Linux RISC-V64 and Windows x64 using the previously documented Zig
host wrapper; foreign executables are not run. See [provenance](PLAYER_RECOVERY.md#validation-of-this-series).
No physical input/cursor, Android/macOS, GPU-loss, endurance, full-core or new
sanitizer acceptance is claimed. All nine roadmap stages remain open.
