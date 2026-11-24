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
disvm --styx-mount-fd 4 --at /flash --share-vfs ./movies -- \
    --native-spawn --no-jit controller.dis
```

`controller.dis` is the local Dis program that controls FlashVM through `/flash/.upload` and `/flash/.state`. Put files it should make available to FlashVM under `./movies`. For example, `./movies/intro/child.swf` is `/intro/child.swf` inside FlashVM. `--share-vfs` grants read and directory traversal. Use `--share-vfs-rw` to also allow writes to existing files where the host grants permission. The shared path is selected by the DisVM host. The retained directory confines paths, excludes symbolic links below the root, and hides physical `.prog` components. The session can retain up to 510 visited ordinary node identities; reopen a session to retire that cache.

The controller sends the startup SWF length as four big-endian bytes followed by the SWF. It writes `start` to `/flash/.state` once to begin. Frame scheduling uses the uploaded header rate, or 12 frames per second when the header rate is zero. The player waits for the next frame or incoming Styx work. `pause` stops frame advances while leaving control files reachable, and `restart` starts again from the uploaded bytes. In this executable `debug` pauses and selects the interpreter. The embeddable owner supports an application debugger hook. Either VM can select `--jit` if its host build supports it.

This is the headless player entry point. The desktop window, debugger frontend, and a typed Dis module catalog are supplied by embedding applications. The same control and VFS protocol applies to each owner.

## Run acceptance

Configure the SDK test build with the matching DisVM source/build and pass `-DSTYXRPC_FLASHVM_PLAYER=/path/to/flashvm_styx_player`. These tests execute the production binaries. They check clock-driven playback, pause, nested SWFs, hidden and out-of-tree paths, and writing an existing file under the explicit write grant.

```sh
ctest --test-dir build/styxrpc -R 'styxrpc-native-player' --output-on-failure
```

[Player implementation](../src/styxrpc/player_main.c) · [DisVM mount command](https://github.com/bhgv/flashVM/blob/master/disVM/runtime/STYX-SERVING.md)
