# styxorderedlaunchfileswithfds

[Русский](LAUNCH-FDS.ru.md) · [Header](launch.h) · [Implementation](launch.c) · [CLI](../../../../runtime/TERMINAL-LAUNCH-CLI.md)

```c
#include "launch.h"

StyxOrderedFiles *styxorderedlaunchfileswithfds(StyxClient *client,
    DVM_LaunchHandoff *handoff, const StyxOrderedOptions *options,
    const StyxLaunchFile *mappings, size_t count,
    const StyxLaunchFd *fds, size_t fd_count);
```

This native POSIX SDK function attaches an ordered broker and prepares a private
terminal VFS projection before DisVM starts. Link `styx_ordered_files`. The
function borrows the accepted terminal client, exclusive prepared handoff,
broker options, and both arrays and their strings through return. Supply the
same broker mappings and count as for [`styxorderedlaunchfiles`](launch.h).
Each `fds` entry has an absolute guest file path, a canonical relative file
name under the handoff's captured terminal root, and a nonzero combination of
`FVM_ACCESS_READ` and `FVM_ACCESS_WRITE`. Counts are in entries. Supply
`NULL, 0` to omit file grants, or up to `STYX_LAUNCH_FDS_MAX` entries.

Each source must resolve to a file during preparation. A file grant cannot
overlap another guest alias or reuse a selected Unibus endpoint. The function
pins each file through an unopened fid. Actual open and I/O rights remain
subject to the configured ceiling and the source provider. If the host uses
a [one-use publication](../../../vfs/HELD-EXPORT.md), the first successful
guest open takes the original file cursor. For example, map
`{"/input", "held-input", FVM_ACCESS_READ}` and call
`sys->open("/input", Sys->OREAD)` in the guest.

Returns an owned file-set on success and replaces the terminal layer in the
handoff. The host calls `styxorderedfilesfree` after guest descriptors and
namespaces retire. A null return leaves the original handoff available.
Invalid paths or rights, missing sources, directories, cancellation and
native setup failures can all cause refusal. Acquisition can wait for Styx.
Call before handing off to the runtime, outside VM and namespace locks.
