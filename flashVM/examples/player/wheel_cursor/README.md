# Wheel and native cursor tutorial

Run the supplied [SWF8](wheel-cursor.swf) in the optional SDL player. Its stopped
320×200 blue-grey scene stays responsive: scrolling traces signed vertical lines;
pressing the primary button hides the cursor; releasing shows it. This demonstrates
input on a stopped timeline and native presentation without extra script frames.
[ActionScript equivalent](wheel_cursor.as), [standalone generator](make_movie.py),
[exact bridge contract](../../../doc/PLAYER_WHEEL_CURSOR.md).

Build the player with AGG and your SDL2 SDK using the
[host/cross-build guide](../../../../BUILDING.md), then from the repository root:

```sh
python3 flashVM/examples/player/wheel_cursor/make_movie.py
build/player/frontend/flashvm_player --interpreter --mute \
  flashVM/examples/player/wheel_cursor/wheel-cursor.swf
```

Use `--jit` where the native backend is available. Escape/close releases the VM,
listener roots, raster surface, retained frame and SDL objects; the host restores
its original cursor state. No audio device is needed. Without a window/device:

```sh
SDL_VIDEODRIVER=dummy build/player/frontend/flashvm_player --interpreter \
  --mute --frames 3 --fps 1000 flashVM/examples/player/wheel_cursor/wheel-cursor.swf
```

This bounded load/run does not inject wheel events or certify physical input.
The automated wheel movie tests do inject real SDL queue messages separately.
For a conventional one-unit wheel step expect trace `3` upward or `-3` downward.
Precise devices can report fractional values. Horizontal-only scroll is ignored.
A physical cursor may not exist on touch hardware. No hit-test target, text-field
scrolling, IME or full Flash object model is added by the example.

| Script API | Effect and ownership |
| --- | --- |
| Mouse.addListener | Registers the observer once; its context owns the strong listener root. |
| onMouseWheel(delta) | Synchronous wheel callback, one numeric argument in host lines; no borrowed native event survives it. |
| Mouse.hide / Mouse.show | Change the context request; player applies it on the SDL owner thread. |
| trace | Prints the supplied delta through the VM trace path. |
| stop | Stops authored timeline playback; it does not terminate the host event loop. |

The generator owns its bytes and uses only Python's standard library. It emits a
small deterministic test movie directly; it is not an AS compiler and does not
change the FIR → SWO → linker → SWF architecture. The .as file explains semantics;
the current numeric FIR source profile cannot compile this object/listener script.
If focus is lost before release, no synthetic release callback is invented; close
or a subsequent press/release restores the native cursor. Cross-built executables
must be run on their own target, with the corresponding target SDL video backend.
