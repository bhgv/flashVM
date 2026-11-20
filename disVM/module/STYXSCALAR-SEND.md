# Styxscalar.send

[Русский](STYXSCALAR-SEND.ru.md) · [Interface](styxscalar.m) · [Implementation](../limbo/styxscalar.b) · [Example](../examples/remote-scalar/STYX.md)

```limbo
send: fn(sys: Sys, fd: ref Sys->FD, tag: big, kind, value: int);
```

Supply a loaded Sys module, a borrowed write FD at the start of the file,
a tag from 1 to 4294967295, a record kind (1 request, 2 value, 3 error)
and the signed integer to encode. Include `sys.m` and `styxscalar.m` and load
the local `styxscalar.dis`.

The blocking function writes one complete S-record. It returns after the
full write is acknowledged. An invalid descriptor or tag raises before
writing. A failed or short write raises an exception with uncertain outcome.
The caller owns the FD and serializes access to it. Cancellation follows
Sys write semantics. The function never resubmits a record.
