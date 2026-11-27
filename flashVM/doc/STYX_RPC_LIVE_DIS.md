# Load a Dis library from the live VFS

[Русский](STYX_RPC_LIVE_DIS.ru.md) · [File handles](STYX_RPC_OS.md) · [Native tasks](ASINC_VFS.md) ·
[DisVM launch options](../../disVM/runtime/TERMINAL-LAUNCH-CLI.md)

In the paired Linux64 player, pass `--live-dis-load` on the DisVM mount
command after `--at` and `--assets`, when supplied. Use a compatible native-cell
`.dis` library under the shared workspace. Enable this session option for a
trusted peer allowed to load and invoke the workspace's Dis code.

In the working directory that contains the library and the Limbo bootstrap,
run the pair with your installed player path:

```sh
disvm --styx-local-pair /path/to/flashvm_styx_player --no-jit -- --live-dis-load -- --no-jit app.dis
```

```actionscript
var source = vfs.open("/library/counter.dis");
var counter = source.load();
source.close();
var answer = counter.next(41);
```

`load()` takes no arguments. On an ordinary `.dis` handle it resolves the
current file through the paired DisVM's VFS, checks its compiled exports
and returns a typed proxy. The byte cursor stays at its original position.
Each successful load owns a distinct instance. A later load resolves a
replacement at the same path, while the first proxy keeps its methods and
mutable state. Closing the source file leaves the proxy usable. The proxy
and any returned channels keep its owner alive until their last reference
is released or the session closes.

The stock playback owner and every `asinc` task have distinct reverse
PIDs and argument buses. The main movie can invoke checked managed, graph
and channel exports directly. A direct call suspends the movie. Its wire
exchange runs on a native thread while the movie owner handles only
native `.prog` requests. Unrelated reverse requests receive a refusal
during the wait, and calls from inside a Styx application callback
cannot start a nested scoped exchange. Use an `asinc` task when the movie
must keep advancing or the Dis function depends on an application callback.

The existing proxy checks the entire advertised interface. An unsupported
contract, a refused image, a missing service or an expired load request
fails the call. The session supports up to 32 concurrent loading or
retained generations, with the underlying Styx walk budget also applying.

Run [the paired test](../../lib/styxrpc/tests/flashvm/mixed_pair.py) with
`--live-dis` and the built player, DisVM, Limbo, AS2, FIR and SWL tools.
Add `--jit` or `--tcp` for those variants. The test rebinds the VFS while
holding an old proxy and checks scalar, string and channel calls.
