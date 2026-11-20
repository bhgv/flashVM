# Vfsnative scalar transport

[Русский](VFS-SCALAR-CALL.ru.md) · [Interface](../module/vfsnative.m) · [Example](../examples/remote-scalar/STYX.md)

The hosted `$Vfsnative` builtin provides shared C transport for the
`fn(int): int` Styx profile. The typed adapter calls the builtin through
its normal Dis call frame. Routes resolve in the active module's VFS, with
cancellable provider operations and independent read and write grants.

| Method | Operation |
| --- | --- |
| `scalarcall(request, reply, requesttag, replytag, value)` | Open the reply, write one call record, and return the scalar reply or raise an exception. |
| `scalartake(path, tag)` | Read and validate one call record and return its scalar argument. |
| `scalarreply(path, tag, success, value)` | Write one value record for success equal to 1 or one error record otherwise. |

Tags are nonzero unsigned 32-bit values and arguments are Limbo integers on
a signed 64-bit wire. This path expects one-shot grants. The owner must call
`scalartake` and `scalarreply` once per grant. Reads retry only an empty
pending result for up to three seconds between operations. A write of unknown
outcome is never automatically repeated. The existing example still uses
typed generated modules to select and invoke the exported function.
