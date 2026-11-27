# styxrpc_channel_invoke_at

[Русский](channel_invoke_at.ru.md) · [API](../include/styxrpc/channel_client.h) · [styxrpc_channel_invoke](channel_invoke.md) · [Module capture](module_open.md)

Pass the borrowed unopened module fid as the first argument, followed by the
arguments of the path-based function. Supply the same scoped target, buffers and result as for the path-based channel call. Returned channels retain their own function and module fids.
The fid determines the target regardless of any path in the descriptor.
Passing NULL selects the ordinary path-based behavior.

The call uses the usual request and reply validation and transfer results.
A captured image withdrawn before function open returns
`STYXRPC_CALL_NOT_SENT` without invoking a replacement at the same path.
Serialize opening and releasing the directory against this call. The
call waits for Styx I/O and respects the supplied reply polling budget.
