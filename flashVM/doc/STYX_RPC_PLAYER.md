# Native paired Styx player

[Русский](STYX_RPC_PLAYER.ru.md) · [Playback protocol](STYX_RPC_MODULES.md) · [File and module APIs](STYX_RPC.md)

The `flashvm_styx_player` executable runs a paired FlashVM owner with a monotonic frame clock. It accepts a connected full-duplex stream on an inherited descriptor at least 3 or opens an explicit TCP endpoint. The executable logs ActionScript `trace` to standard output. Build it with the installed hosted Styx and libstyxrpc SDK.

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

## Start over TCP

Run the listener on the FlashVM machine. Set `HOST` to a local interface's
address and `PORT` to a decimal port. Loopback is suitable when both processes
run on the same host. A reachable interface can serve a peer on another host.

```sh
flashvm_styx_player --listen 127.0.0.1 7474 --jit --window
disvm --styx-mount-tcp 127.0.0.1 7474 --at /rpc -- --native-spawn --jit desktop-paired.dis
```

`--listen HOST PORT` accepts one connection and closes the listening socket.
Use port `0` to let the kernel choose. The first output line is
`styx listening PORT` with the actual port. `--connect HOST PORT` dials a
listener. Connect attempts share a five-second budget after name resolution.
The equivalent `--fd FD` accepts a stream supplied by a supervisor. These
choices all use the same StyxRPC session. The direct TCP carrier is plaintext
and relies on a trusted network or an independently protected stream.

See the [paired IDE guide](../../ide/DESKTOP-PAIRED.md) for the application
modules and asset layout, and the [DisVM TCP reference](../../disVM/runtime/TERMINAL-LAUNCH-CLI.md#tcp-endpoints)
for serving a remote named program or library.

## Connect two FlashVMs

Choose complementary roles so each mounted client reaches the other player's
server. The TCP listener and dialer can take either role.

```sh
flashvm_styx_player --listen 127.0.0.1 7474 --peer forward --movie view-a.swf --jit
flashvm_styx_player --connect 127.0.0.1 7474 --peer reverse --movie view-b.swf --jit
```

`forward` serves lane 0 and mounts lane 1. `reverse` serves lane 1 and mounts
lane 0. Each role serves version and attach requests while its own mount
negotiates. `--movie FILE` in peer mode loads a local startup SWF and starts
it at the first clock turn after the connection is attached. Omit `--movie`
to wait for upload through the peer connection. Each player retires its
session when the connection closes. To use an externally supplied transport,
substitute `--fd FD` for each TCP endpoint.

Each movie can [publish an object](#publishing-and-using-movie-modules) and use
[`styxrpc.callInt`](STYX_RPC_SESSION_CALL_INT.md) to call a published Int32
method on the other movie. The two films own separate heaps. The session
mount in this mode contains the other FlashVM's playback and published image.
Font lookup through the Dis VFS belongs to the DisVM launch mode.

## Unibus terminal

[Application exchange through the default rpc object](STYX_RPC_DEFAULT.md) is available to [typed Limbo clients](../../disVM/runtime/STYX-CHANNEL.md).

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

## System file handles

Movies with a mounted VFS can use the [rpc and os file methods](STYX_RPC_OS.md).

## Publishing and using movie modules

A movie can expose several typed callbacks and live Int32 properties on a
single Dis-shaped image. See [publishObject](STYX_RPC_PUBLISH_OBJECT.md).
Use [loadModule](STYX_RPC_LOAD_MODULE.md) and
[intVariable](STYX_RPC_INT_VARIABLE.md) on the attached session. For a custom
UniBus exchange, use [openMessages](STYX_RPC_OPEN_MESSAGES.md).

Embedders pass `fvm_styxrpc_playback_slot(playback)` as the tree context for
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

Run the two-player acceptance test with the project AS2 compiler, FIR and
SWL. It links the provider and caller SWFs, tests both lane assignments in
JIT and interpreter, and checks peer retirement on disconnect. `--tcp` selects
a direct TCP loopback connection. The default uses a local socket pair.

```sh
python3 ide/tests/check_flash_peers.py --tcp \
  --player build/flash/flashvm_styx_player --as2 build/as2/view/as2_viewc \
  --fir build/fir --swl build/linker/swl
```

The [paired IDE check](../../ide/DESKTOP-PAIRED.md) has the same `--tcp` switch.

[Player implementation](../src/styxrpc/player_main.c) · [DisVM mount command](https://github.com/bhgv/flashVM/blob/master/disVM/runtime/STYX-SERVING.md)
