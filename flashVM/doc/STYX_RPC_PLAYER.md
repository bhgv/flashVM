# Native paired Styx player

[Русский](STYX_RPC_PLAYER.ru.md) · [Playback protocol](STYX_RPC_MODULES.md) · [File and module APIs](STYX_RPC.md)

The `flashvm_styx_player` executable runs a paired FlashVM owner with a monotonic frame clock. It accepts one already connected, full-duplex stream on an inherited descriptor at least 3. The two directions of the existing Styx connection carry FlashVM controls and the DisVM VFS. The executable logs ActionScript `trace` to standard output. Build it with the installed hosted Styx and libstyxrpc SDK.

```sh
cmake -S flashVM -B build/flash -DFVM_ENABLE_STYXRPC=ON -DCMAKE_PREFIX_PATH=/path/to/sdk
cmake --build build/flash --target flashvm_styx_player
```

## Start the two owners

Create a connected socket pair in your process supervisor and pass one end to each command below. The numbers are the inherited descriptors in the respective processes. The supervisor closes its copies after spawning.

```sh
flashvm_styx_player --fd 3 --no-jit
disvm --styx-mount-fd 4 --at /flash -- \
    --native-spawn --no-jit controller.dis
```

The Dis program `controller.dis` controls FlashVM through `/flash/.upload` and `/flash/.state`. FlashVM resolves ordinary paths in the running Dis program’s VFS from its namespace root. Bindings and their order are consulted at each new lookup. Use the DisVM working directory at launch as the initial namespace root and change bindings in DisVM as needed. Ordinary access and writes use the effective permissions of the chosen provider.

The controller sends the startup SWF length as four big-endian bytes followed by the SWF. It writes `start` to `/flash/.state` once to begin. Frame scheduling uses the uploaded header rate, or 12 frames per second when the header rate is zero. The player waits for the next frame or incoming Styx work. `pause` stops frame advances while leaving control files reachable, and `restart` starts again from the uploaded bytes. In this executable `debug` pauses and selects the interpreter. The embeddable owner supports an application debugger hook. Either VM can select `--jit` if its host build supports it.

The player starts headless by default. Build the raster backend and the
stock SDL2 frontend to use a window. For example, from the repository root:

```sh
cmake -S flashVM -B build/flash-window \
  -DFVM_ENABLE_STYXRPC=ON -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" \
  -DCMAKE_PREFIX_PATH=/path/to/sdk
cmake --build build/flash-window --target flashvm_styx_player
```

Pass `--window` on launch. The window opens after upload and sends native
keyboard and mouse events to the SWF's ordinary input handlers. Presentation
continues while the timeline is paused, so methods called from DisVM can
still update the scene. Closing the window ends the session.
Pass `--capture frame.ppm` on a raster-enabled build to save the final frame
on normal shutdown, with or without a window.

Movies that forward input to another owner can opt in to the
[window input FIFO](STYX_RPC_HOST_INPUT.md) with `hostInput.enable()`.

The `text-jit.defs` profile adds standard TextField rendering. At movie setup
the player opens `fonts/NotoSansMono-Regular.ttf` through the same DisVM VFS
session. An absent file leaves the text library unavailable. A present font
must be valid TTF and no larger than 4 MiB. Each movie owns its font copy.

## ActionScript session

Uploaded movies receive `styxrpc` and `styxChannel`. The `styxrpc` object
uses the mounted DisVM session. For example, `styxrpc.read("project.txt", 0, 32)`
returns a status and a byte array. `styxChannel("(i)", 1)` creates a local
typed queue. Each new movie owns its own session object and replacement
closes the previous movie’s launch fids.

## Unibus terminal

In a bidirectional launch, the Limbo program registers its
`chan of array of byte` with `$Unibus.attach(input, output)` and disconnects
them with `detach()`. The SWF receives `styxTerminal` with `bidir == true`.
`trySendFrame(frame)` queues a single counted fvd1 string and returns true
on acceptance. A false result permits a later attempt. `tryReceiveFrame()`
returns the next string or undefined. A movie can poll these methods
from `onEnterFrame`. Each direction has space for one frame. Consumption
grants credit for the next frame.

Define a root method `onTerminalFrame()` and call
`styxTerminal.enableNotifications()` to receive a turn after the player has
copied an inbound frame. The player calls the method with no arguments between
Styx dispatches while playback is running. Read the frame through
`tryReceiveFrame()` to return credit. A frame left unread can trigger another
turn. The callback runs on the FlashVM owner alongside the movie clock.

The [IDE terminal contract](../../ide/TERMINAL-BIDIR.md) shows how to pass
this object to the existing ActionScript adapter.

## Publishing a movie object

Call `styxrpc.publishObject("view.dis", receiver, ["key", "onKey"])`. DisVM
loads the image with ordinary `load` at `/flash/mounted/view.dis`. In this
example Limbo declares `key: fn(code: int): int` and AS implements
`onKey`. Supply up to eight method descriptors. A managed method takes five
fields, for example `["label", "describe", 0xac26a778, "(s)", "((fs))"]`
for `fn(string): (real, string)`. Use the compiled Dis signature and
the ordinary libstyxrpc codec for shaped values.

A movie can publish one object image. A replacement can publish a fresh image
with its own fids. Fids of the withdrawn image return an error. Embedders pass
`fvm_styxrpc_playback_slot(playback)` as the tree context for
`styxrpc_module_slot_tree`.

## Run acceptance

Configure the SDK test build with the matching DisVM source/build and pass `-DSTYXRPC_FLASHVM_PLAYER=/path/to/flashvm_styx_player`. These tests execute the production binaries. They check clock-driven playback, pause, live VFS bindings, nested SWFs, calls to the movie’s object and retirement of old fids.

```sh
ctest --test-dir build/styxrpc -R 'styxrpc-native-player' --output-on-failure
```

Set `STYXRPC_FLASHVM_WINDOW_PLAYER` to the SDL-enabled executable. The
`styxrpc-native-window-*` tests use SDL's dummy driver and check the new
and old positions of a shape moved by a DisVM call.

The `styxrpc-native-terminal-*` tests run a `.dis` and `.swf` on the
stock pair. A key frame reaches a Limbo channel and its Unicode reply
updates the raster in the windowed run.

[Player implementation](../src/styxrpc/player_main.c) · [DisVM mount command](https://github.com/bhgv/flashVM/blob/master/disVM/runtime/STYX-SERVING.md)
