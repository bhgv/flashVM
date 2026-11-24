# Paired FlashVM and DisVM

[Русский](STYX_RPC_MODULES.ru.md) · [Styx RPC integration](STYX_RPC.md) · [Ordinary files](STYX_RPC_FILESYSTEM.md) · [Run the native paired player](STYX_RPC_PLAYER.md)

## Use the DisVM filesystem and load a module

Mount the attached DisVM session at `/` with [the Styx filesystem mount](STYX_RPC_FILESYSTEM_MOUNT.md). Do this in a fresh FlashVM context, before installing `fs`. Both relative and absolute script paths resolve from the DisVM root. A failed remote lookup returns its error. The root stays bound until context teardown. File channels share this namespace. The ordinary view omits `.prog` before applying list pagination and refuses paths containing that component. A host may still bind an explicitly authorized typed variable through [the external variable API](STYX_RPC_INT_VARIABLE_BIND.md).

Describe the intended imports with the compiled signatures and schemas from the Dis module. Use [the native loader](STYX_RPC_MODULE_LOAD.md) to bind an already loaded module object into ActionScript, or [install a script loader](STYX_RPC_MODULE_LOADER_INSTALL.md) with a catalog of known interfaces. For example, with a host catalog declaring `int next(int)` on the configured remote path, the following calls a native Dis function over the attached session.

```actionscript
var dis = loadDis("/mounted/limbo.dis");
trace(dis.next(41)); // 42 with the test module
```

Module entries are ordinary ActionScript object methods. The SDK verifies the compiled contract through a held directory before creating the object. Each invocation uses the existing checked Styx/UniBus call profile. The returned object owns its descriptors. The session and reverse channel bus are borrowed from the host.

## Publish a live Flash object

1. Create a live ActionScript object and choose its public method and environment names. Give each method a compiled signature and one scalar, managed, positional graph or channel factory profile.
2. Pass the object and the descriptors to [the image constructor](STYX_RPC_OBJECT_IMAGE_CREATE.md). It copies the declarations and keeps the receiver rooted. Each invocation resolves the current method on that receiver. A module may offer up to eight functions and eight explicit signed integer properties under `.var`.
3. Serve [the backing module](STYX_RPC_OBJECT_IMAGE_MODULE.md) with the hosted SDK module tree. DisVM mounts the session and can use ordinary typed Limbo `load` for `mounted/<name>.dis` under that mount. Close the server before [destroying the image](STYX_RPC_OBJECT_IMAGE_DESTROY.md).

The image looks like a loaded module directory. It contains `.interface`, function files and the published environment. The independently installable C SDK builds the same image from callbacks supplied by any program. Refer to the [C module contract](https://github.com/bhgv/flashVM/blob/master/lib/styxrpc/reference/module_create.md) and the [executable Limbo client](https://github.com/bhgv/flashVM/blob/master/lib/styxrpc/tests/flashvm/object_image_client.b).

## Load and control SWFs

The host creates [a playback owner](STYX_RPC_PLAYBACK_CREATE.md) and publishes [its control tree](STYX_RPC_PLAYBACK_CONTROL.md), standalone or alongside modules at the same server root. The first and any replacement SWF use the same file.

| File | Operation | Contract |
| --- | --- | --- |
| `.upload` | Write | Open a new write fid. Send the SWF byte count as an unsigned four-byte big-endian prefix, then the SWF bytes at consecutive offsets. The final write commits a verified movie in `ready`. |
| `.state` | Write | Use one command per open at offset zero. Send `start`, `pause`, `restart` or `debug`, with an optional final LF. |
| `.state` | Read | Read a newline-delimited `key=value` snapshot. Reads at different offsets on the same fid refer to the same snapshot. Reopen for new measurements. |

Only one upload fid may be open at a time. Closing a partial upload discards it. An invalid upload leaves the previous movie available. `start` and `restart` advance the first tick immediately, and the host schedules [subsequent ticks](STYX_RPC_PLAYBACK_TICK.md) on the same owner. `pause` suspends ticks. `restart` creates a fresh context from the stored bytes and rebuilds the paired root if configured. `debug` pauses in interpreter mode and calls the host's debugger attach hook if configured. The application starts and retires the debugger separately and [destroys playback](STYX_RPC_PLAYBACK_DESTROY.md) after its server.

The snapshot reports the state, last result, completed ticks, SWF and string memory in bytes, GC counts, and interpreter and JIT counters. On Linux it also reports resident bytes, process thread count and up to 48 native thread CPU percentages measured between reads. The OS process includes host and transport threads. CPU rates require a prior sample. The owner thread percentage is available where the POSIX thread CPU clock is supported. The host may append application-specific lines.

This bridge provides an embeddable owner. The application supplies the timeline clock and debugger and can install further bindings on each new context. The desktop player has its own startup path. Legacy script worker spawning is disabled while an exclusive root is installed because those workers create independent filesystem namespaces.

## Load further SWFs from ActionScript

A paired [playback owner](STYX_RPC_PLAYBACK_CREATE.md) automatically installs AVM1 movie loading when its spec contains the attached Dis session. In an uploaded movie, create a target clip and load a file published at the remote path.

```actionscript
var slot = _root.createEmptyMovieClip("slot", 1);
slot.loadMovie("/movies/chapter/child.swf");
```

The same bridge handles global `loadMovie(url, target)`, AVM1 `GetURL2` movie-target requests, and `loadMovieNum(url, 0)`. The startup SWF uses the attach root for relative names because its upload contains no filename. A child has the canonical path it was fetched from. If `/movies/chapter/child.swf` requests `../leaf.swf`, the loader opens `/movies/leaf.swf`. A leading slash always selects the same attached VFS root. The paths accept literal UTF-8 bytes. URL schemes, query and fragment syntax, paths escaping the attach root and the private `.prog` component are refused.

The load captures the destination clip and its source during the calling frame. Between frames, the owner reads a complete SWF through one open Styx fid and publishes it only after validation. The loaded clip can execute on the next tick. The loader can queue up to 16 requests in one frame. Each file is limited by the smaller of the playback upload limit and the default SWF size limit. Missing, malformed or oversized files leave the clip unchanged and are recorded in `.state` as `movie_load_errors` and `last_movie_result`. Successful replacements increment `movies_loaded`. A new upload or restart resets these counters. The loader uses only the attached VFS for acquisition.

This profile covers AVM1 clip and level-zero loading of supported SWFs. Numbered levels above zero, HTTP methods other than the accepted GET spelling, variable loading and the MovieClipLoader event API are outside this binding. The host still provides the playback clock. The native `playback-movies` checks execute two levels of SWF actions and include a real local file with a name missing from the attached tree.

Run the native checks after configuring the SDK tests with the DisVM build and the FlashVM test host.

```sh
ctest --test-dir build/styxrpc -R 'object-image|module-load|playback' --output-on-failure
```
