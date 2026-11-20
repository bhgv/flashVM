# styxheldfdmap

[Русский](HELD-FD-MAP.ru.md) · [Header](held_export.h) · [Implementation](held_export.c) · [Open-file export](HELD-EXPORT.md) · [Launch CLI](../../runtime/TERMINAL-LAUNCH-CLI.md)

```c
#include "held_export.h"

int styxheldfdmap(DVM_View *view, unsigned number, DVM_Provider *grant, unsigned access);
```

The native POSIX `styx_vfs` SDK maps an already prepared file grant into a
private launch tree at `/.fd/NUMBER`. The number is decimal, from 0 through
31. Supply a mutable view builder and a live root file grant returned by
[`styxheldexport`](HELD-EXPORT.md). The `access` argument is a nonempty
combination of `FVM_ACCESS_READ` and `FVM_ACCESS_WRITE` permitted by that grant.
The builder creates the directory as part of the VFS projection. Build a
separate view for each launch and publish that view to the intended peer.

Returns 1 when the view retains the mapping, or 0 on refusal or allocation
failure. The caller retains its own view and grant ownership. A successful
mapping follows the regular view rule for replacing the same path. The call
checks effective access without opening the file. Builder mutations require
exclusive access. Freeze the view before sharing it with a Styx server.

For example, map an existing read grant at number 0 and configure the receiver
with `--inherit-fd 0 r`. After the receiver opens it during startup, Limbo
obtains its own descriptor via `sys->fildes(0)`.
