# vfs.rmnode

[Русский](STYX_RPC_RMNODE.ru.md) · [Publication](STYX_RPC_NEWNODE.md) · [Default session](STYX_RPC_DEFAULT.md) · [C SDK](../../lib/styxrpc/reference/module_slot_remove_node.md) · [Implementation](../src/styxrpc/player_services.c#L189)

```actionscript
removed = vfs.rmnode(path)
```

Withdraw a callback file published by the active movie through
[`vfs.newnode`](STYX_RPC_NEWNODE.md). In the paired player, `os.rmnode`
uses the same publication. `path` is a UTF-8 name relative to the
movie's exported root, with an optional leading slash. It follows the
publication path rules. It accepts at most 1024 bytes excluding the
leading slash. The current directory has no effect.

The result is `true` when the file is withdrawn. Existing Styx fids for
that file stop accepting data and metadata requests. Empty directories
created for the file are pruned. Other files and nonempty directories
keep their identities. A later publication at the same path gets a new
identity. Open it again to reach the replacement. Open directory
enumerations can require a restart when the publication tree changes.

The result is `false` for an absent file, directory, reserved path or
an attempt from inside a callback. Missing, wrongly typed, empty or overlong arguments raise an error. An
embedded NUL is also invalid. Call on the owner of the active movie in an attached
paired player. Withdrawal is synchronous and local. The method releases
the published function without asking the other VM to remove anything.
Use [`vfs.rm`](STYX_VFS_RM.md) for the peer's ordinary VFS.

```actionscript
if (vfs.rmnode("/app/value")) {
    vfs.newnode("/app/value", "r", readNewValue);
}
```

In the [paired check](../../disVM/runtime/STYX-CHANNEL.md#acceptance),
add `--withdraw`. The [AS movie](../../lib/styxrpc/tests/flashvm/withdraw_pair.as)
withdraws a file while a [Limbo caller](../../lib/styxrpc/tests/flashvm/withdraw_pair.b)
holds it and its sibling open. The caller sees the replacement only
after a fresh open.
