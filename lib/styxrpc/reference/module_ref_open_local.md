# styxrpc_module_ref_open_local

[Русский](module_ref_open_local.ru.md) · [API](../include/styxrpc/module_ref.h#L54) · [Source](../src/module_ref_local.c#L63) · [Guide](module_refs.md)

`StyxRpc::serve`

```c
StyxRpcStatus styxrpc_module_ref_open_local(struct StyxRpcModuleSlot *slot,
    const StyxRpcModuleRef *ref, StyxRpcFile *out);
```

## Contract

Call on the owning thread of a live module `slot`. Pass its [decoded export descriptor](module_ref_decode.md) and a zero-initialized `out`. The call resolves the canonical path in the current publication, checks the exact Qid.path, confirms a four-byte regular value and requires both the descriptor's mode and the tree's permissions.

Success returns `STYXRPC_OK` and opens a tracked file in `out`. Invalid input, refusal or a stale Qid returns `STYXRPC_INVALID` and preserves `out`. Module replacement retires the underlying variable access even while the handle is held. Use [local read](module_ref_read_local.md) and [write](module_ref_write_local.md) on the slot owner thread. Close with `styxrpc_file_close` before destroying the slot. The operation performs no transport I/O.
