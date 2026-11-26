# Styxexec.call

[Русский](styx-exec-call.ru.md) · [Guide](STYX-EXEC.md) · [Interface](../module/styxexec.m#L19) · [Source](../limbo/styxexec.b#L48)

```limbo
call: fn(mount, path: string, input: array of byte): array of byte;
```

Call this method after [init](styx-exec-init.md). Give it an absolute
mount such as `/peer`, the nonempty path of an executable node relative
to that root and the borrowed argument bytes. The path limit is 1024
UTF-8 bytes. Input is limited to 65,536 bytes and may be empty.

The method opens a private fid, waits for the callback and returns
its reply in a new array. The result can be empty and fits in the
input extent. The input array remains caller-owned. Invalid paths,
provider refusal and failed transport operations raise exceptions.
Do not rely on an exception as evidence that the callback never ran.
The method never replays a call. Processes can issue simultaneous
calls through the initialized module using separate fids.

```limbo
reply := exec->call("/peer", "tools/apply", array[] of {byte 255, byte 40, byte 0, byte 23});
```

The result is three bytes in the [paired test](../../lib/styxrpc/tests/flashvm/execute_pair.b).
