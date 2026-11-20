# Scalar interface admission over Styx

[Русский](SCALAR-INTERFACE.ru.md) · [Header](scalar_interface.h) · [Test](tests/scalar/route.c)

The shared C code carries one export name of up to 48 bytes and its exact
32-bit compiled Dis signature. The publisher supplies a checked compiled
export and the caller compares it with its compiled import. The record fits
in the existing one-shot `.unibus` inbox.

In `styx_terminal`, `styxscalaroffer` takes a borrowed write file, a nonzero
tag, a name and a signature. It returns the delivery status of one write.
`styxscalaraccept` checks the tag, record integrity, name and signature
without I/O. `styxscalarcallchecked` also takes a call tag and one signed
argument. It returns `DVM_LAUNCH_NOT_SENT` on mismatch before opening the
request file. Uncertain writes are never replayed.

The native test exercises these functions. Ordinary `load`, `.m` reading
and Dis export dispatch are integration work.
