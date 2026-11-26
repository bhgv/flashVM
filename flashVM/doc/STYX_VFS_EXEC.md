# vfs.exec

[Русский](STYX_VFS_EXEC.ru.md) · [Session](STYX_RPC_DEFAULT.md) · [AS implementation](../src/styxrpc/session_exec.c#L16) · [C SDK](../../lib/styxrpc/reference/node_exec.md)

```actionscript
reply = vfs.exec(path[, bytes])
```

Call a peer callback published as an executable node through
[`vfs.newnode`](STYX_RPC_NEWNODE.md) or the C SDK. In the paired player
`os.exec` invokes the same method on the same object. The invocation
uses the attached libstyxrpc session.

| Argument | Meaning |
| --- | --- |
| `path` | VFS path up to 1024 bytes after normalization, excluding the leading slash. Absolute paths start at the peer root. Relative paths start at the session's [working directory](STYX_VFS_PWD.md). The root itself and reserved components are invalid. |
| `bytes` | Optional dense array of exact numbers from 0 through 255. The maximum length is 65,536 bytes. An omitted or empty array sends zero bytes. |

The result is a new dense Array with the confirmed reply. Its length fits
in the input extent and may be zero. The method snapshots the input
before sending and leaves it untouched. Reading the length and indexed
properties can run getters before submission.

Invalid arguments fail before any remote request. Service admission,
provider refusal and transport errors raise exceptions. A failed reply
can follow an execution. The SDK never replays a call automatically.
The method blocks the movie's owner thread. A peer callback should reply
before making a synchronous call back into the waiting movie.

The paired FlashVM player and hosted libstyxrpc slot servers provide the
executable service. In DisVM, the [Limbo client](../../disVM/runtime/STYX-EXEC.md)
calls that service. A stock DisVM provider publishes ordinary files and
[Dis module methods](STYX_RPC_LOAD_MODULE.md).

```actionscript
os.cd("/tools");
var args = [255, 40, 0, 23];
var result = vfs.exec("apply", args);
```

The [paired AS handler](../../lib/styxrpc/tests/flashvm/execute_apply.as)
returns the invocation count and both endpoints of the input. The
[test caller](../tests/styxrpc/peer-exec-caller.as) exercises both method
spellings. Add `--exec-only` to the [two-player check](STYX_RPC_PLAYER.md)
to run this scenario by itself.
