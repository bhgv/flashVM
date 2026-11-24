# Hosted module images and control files

[Русский](MODULES.ru.md) · [SDK home](README.md)

Link `StyxRpc::serve` for the hosted callback trees. Describe each export with its compiled Dis signature, its schema when the profile uses one, an ordinary C callback and a context. [Create the image](reference/module_create.md) and hand it to the Styx server as the context of `styxrpc_module_tree`. The image exposes one `.interface`, a file for each method, any markers required by the selected profiles, and optional live signed integer cells in `.var`. Limbo clients can use typed `load` against the mounted directory. [Check an interface](reference/module_check.md) from another hosted C program before binding callable imports.

A host can also publish [control files](reference/control_create.md) for an opaque program or device image. Serve `styxrpc_control_tree` on its own or pass the control descriptor in the module spec to place both trees in the same session. `.upload` receives an unsigned 32-bit big-endian payload length followed by exactly that many bytes. Writes start at offset zero and must be consecutive. The final write succeeds only if the owner accepts the complete payload. Closing a partial upload discards it. Read `.state` for a per-fid text snapshot. Write one of `start`, `pause`, `restart` or `debug` at offset zero, with optional LF. The owner supplies payload validation, transitions, statistics and any scheduler. Use an authorized transport. The current server does not authenticate or grant sessions.

Close the server to retire open fids before [destroying the module](reference/module_destroy.md) or [the control descriptor](reference/control_destroy.md). Callback execution is synchronous on the owner. Nothing in these trees requires linking either VM.

Use `StyxRpc::launch` for the [ordinary VFS client](reference/vfs_stat.md). It suppresses `.prog` from each page and refuses any ordinary path containing that component. [Typed integer open](reference/vfs_int_open.md) can open an explicit authorized `.var` cell. Existing typed channel and program RPC paths keep their own admission contracts.
