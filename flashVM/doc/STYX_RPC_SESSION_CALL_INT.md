# styxrpc.callInt

[Русский](STYX_RPC_SESSION_CALL_INT.ru.md) · [Session binding](STYX_RPC_SESSION.md) · [Implementation](../src/styxrpc/session_calls.c)

```actionscript
result = styxrpc.callInt("mounted/peer.dis", "magnitude", -12);
```

Call an exported `fn(int): int` on the current mounted session. `module` is a
relative path to a published module. `method` is its single named export
component. Use an exact signed 32-bit integer for `argument`. The example
returns 12 when the peer exports `Math.abs` under `magnitude`.

The call borrows the session and verifies the compiled signature from the
held module before writing its request. It returns an exact signed 32-bit
number or raises a script error for malformed arguments, refusal before
submission, remote failure, or uncertain outcome. An uncertain outcome may
already have executed on the peer. The call closes its function fid on return.

This operation blocks the calling movie owner. The reply polling budget is
1,000 ms in addition to the mount's I/O deadlines. Schedule calls so the
peer owner is available to dispatch them. Simultaneous calls that each wait
for the other owner can time out.

[Player commands and acceptance](STYX_RPC_PLAYER.md#connect-two-flashvms) ·
[Shared scalar client](../../lib/styxrpc/reference/scalar_invoke.md) ·
[Caller AS fixture](../tests/styxrpc/peer-caller.as) · [Provider AS fixture](../tests/styxrpc/peer-provider.as)
