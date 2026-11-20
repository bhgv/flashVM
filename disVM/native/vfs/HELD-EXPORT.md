# styxheldexport

[Русский](HELD-EXPORT.ru.md) · [Header](held_export.h) · [Implementation](held_export.c) · [Open fid lifetime](OPEN-FID-LIFETIME.md) · [Single-file Styx bridge](SINGLE-FILE.md)

```c
#include "held_export.h"

DVM_Provider *styxheldexport(DVM_VfsHandle *source, unsigned access);
```

This function in the native POSIX `styx_vfs` SDK turns one already-open file into a
provider with a single root file (`.`). The host calls it after all borrowers of
`source` have retired. Pass a nonzero combination of `FVM_ACCESS_READ` and
`FVM_ACCESS_WRITE` that the caller actually holds on that open. The provider
checks this ceiling on each attempt, independently of displayed mode bits.

A non-null result is an owned provider reference and leaves `source` empty.
A null result leaves the handle owned by the caller. Invalid rights, a directory,
missing file metadata or I/O callbacks, and failures during native setup can
cause rejection. The call may block while obtaining metadata, so invoke it
outside VM and namespace locks. Release the returned reference with
`dvm_provider_release`.

The first permitted open takes the original object and cursor. Supported modes
are `r`, `wo`, and `rw`. A mode refusal leaves the grant available. Concurrent
opens can have only one winner, and closing the winner never re-arms the grant.
The returned stream supports sequential I/O. In Styx, its wire offset begins
at zero and advances by the acknowledged byte count from the existing source
cursor. Mode and length are snapshots of the underlying open. Length describes
the underlying file, so reads determine EOF from the current cursor.

Pass the returned provider to `styxbacking` to serve the file using the existing
Styx handlers. The host controls who can reach the published route. The peer
opens that route to obtain its own fid. An accepted open survives removal of
the publication. The open owns close after acceptance, and final provider
retirement queues an unclaimed close on a detached worker. Each published grant
uses one such worker. This interface accepts host-owned native handles.
