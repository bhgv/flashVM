# Styxscalar.call

[Русский](STYXSCALAR-CALL.ru.md) · [Interface](styxscalar.m) · [Implementation](../limbo/styxscalar.b) · [Runnable example](../examples/remote-scalar/STYX.md)

```limbo
call: fn(request, reply: string, requesttag, replytag: big, value: int): int;
```

Include `styxscalar.m` and load the matching local `styxscalar.dis`.
`request` and `reply` are granted paths for writing and reading respectively.
Both tags are unsigned integers from 1 to 4294967295. The argument is a Limbo
`int`, encoded in signed 64-bit wire form. The reply value must fit the
caller's `int`. The module holds no persistent state and the two opened
descriptors are scoped to the call.

This blocking call opens both files before submission and attempts one record
write. After acknowledgement, it waits for a reply with [Styxscalar.take](STYXSCALAR-TAKE.md).
The host serializes callers and supplies a fresh one-shot reply route. Open
failure means the request was never submitted. A failed or short write, missing
reply, invalid frame or cancellation can leave execution uncertain. A validated
owner error raises `remote scalar owner error`. The routine never retries.
Cancellation follows the ordinary Sys file operations of the mounted provider.

Related [Styxscalar.send](STYXSCALAR-SEND.md).
