# styxrpc_scalar_invoke_at

[Русский](scalar_invoke_at.ru.md) · [API](../include/styxrpc/scalar_client.h) · [styxrpc_scalar_invoke](scalar_invoke.md) · [Module capture](module_open.md)

Pass the borrowed unopened module fid as the first argument, followed by the
arguments of the path-based function. Pass the exact export name, its compiled Int32 signature, argument, reply budget in milliseconds and the output pointer.
The fid determines the target regardless of any path in the descriptor.
Passing NULL selects the ordinary path-based behavior.

The call uses the usual request and reply validation and transfer results.
A captured image withdrawn before function open returns
`STYXRPC_CALL_NOT_SENT` without invoking a replacement at the same path.
Serialize opening and releasing the directory against this call. The
call waits for Styx I/O and respects the supplied reply polling budget.
