# Styxscalar.take

[Русский](STYXSCALAR-TAKE.ru.md) · [Interface](styxscalar.m) · [Implementation](../limbo/styxscalar.b) · [Example](../examples/remote-scalar/STYX.md)

```limbo
take: fn(sys: Sys, fd: ref Sys->FD, tag: big): (int, int);
```

Load the matching local `styxscalar.dis` after including `sys.m` and
`styxscalar.m`. Supply a loaded Sys module, a borrowed read FD at the start of
a one-shot record and its tag (1 to 4294967295). The function validates the
frame, checksum, tag and receiving ABI's `int` range. It returns the record
kind (1 request, 2 value or 3 error) and the value. The consumer checks which
kind is appropriate.

Only the native inbox's pending status retries a read, with 10 ms pauses
and a 3000 ms budget checked between reads. Provider operations can have
their own blocking limits. Other read failures, timeout, invalid frames
and invalid handles raise an exception. The caller retains the FD and
must serialize readers that share its cursor. No write or request replay
occurs.
