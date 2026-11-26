# Executable callback nodes from Limbo

[Publish handlers from Limbo](STYX-EXEC-PUBLISH.md)

[Русский](STYX-EXEC.ru.md) · [Provider](../../flashVM/doc/STYX_RPC_NEWNODE.md) · [C SDK and Styx exchange](../../lib/styxrpc/reference/node_exec.md)

Use `Styxexec` when a peer exposes one or more executable entries under
its VFS. FlashVM publishes those entries from ordinary AS functions with
`vfs.newnode(path, "x", callback)`. The provider can group related
functions in a directory. A mount such as `/peer` already holds the
bidirectional session. Calls use that mount and resolve each target from
its root, independently of either program's working directory.

## Run the paired example

1. Use the same [executables and compiler arguments](STYX-CHANNEL.md#acceptance)
   as for the paired channel test and add `--execute`.
2. The runner builds [the Limbo application](../../lib/styxrpc/tests/flashvm/execute_pair.b)
   and three AS source units. [The frame method](../../lib/styxrpc/tests/flashvm/execute_pair.as)
   publishes two executable entries and one read-only entry.
3. [The handler](../../lib/styxrpc/tests/flashvm/execute_apply.as) sees
   `"x"`, a byte Array and its size. It replaces the first bytes with the
   invocation count and the two input endpoints, then returns the output
   length. The test checks refusal, an empty request and a request split over
   transport writes. The expected line is `PASS native execute RPC paired VMs`.

Use `--tcp` for the TCP pair and `--jit` for the JIT profile. The callback
starts on the movie's owner thread when all input bytes arrive. Its reply
fits in the argument buffer. Ordinary directory enumeration hides the
service file. The executable node is visible at its published path.

Use [init](styx-exec-init.md) to load the local Sys interface and
[call](styx-exec-call.md) to invoke an entry.
