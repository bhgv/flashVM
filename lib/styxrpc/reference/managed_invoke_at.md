# styxrpc_managed_invoke_at

[Русский](managed_invoke_at.ru.md) · [API](../include/styxrpc/managed_client.h) · [styxrpc_managed_invoke](managed_invoke.md) · [Module capture](module_open.md)

Pass the borrowed unopened module fid as the first argument, followed by the
arguments of the path-based function. Supply the managed target, its encoded argument children and the reply buffer as for the path-based call. The decoded result borrows that buffer.
The fid determines the target regardless of any path in the descriptor.
Passing NULL selects the ordinary path-based behavior.

The call uses the usual request and reply validation and transfer results.
A captured image withdrawn before function open returns
`STYXRPC_CALL_NOT_SENT` without invoking a replacement at the same path.
Serialize opening and releasing the directory against this call. The
call waits for Styx I/O and respects the supplied reply polling budget.

[Cancellable variant](managed_invoke_at_until.md).
