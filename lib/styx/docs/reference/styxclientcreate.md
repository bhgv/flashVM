# styxclientcreate

[Русский](styxclientcreate.ru.md) · [Header](../../client/client.h) · [Source](../../client/open.c) · [Open an existing fid](styxclientopen.md)

```c
#include <client.h>
int styxclientcreate(StyxFid *directory, const char *name, unsigned permissions,
    int mode, StyxTransfer *result);
```

Link the hosted `Styx::client` target. Provide an unopened directory
fid, a nonempty child name of fewer than `STYX_NAME_MAX` bytes, mode bits
within 0777, an access mode from 0 to 2, and storage for the transfer
result. The child name cannot contain a slash or be `.` or `..`.

The call sends a single `Tcreate` and waits up to the request deadline.
On success it returns one, and the supplied fid now identifies the opened
child. The caller still owns the fid. On failure it returns zero. A
missing or unusable acknowledgement can fence further operations on
the fid. Inspect `result` before retrying and clunk the fid when done.
Serialize use of the fid with other operations and clunk.
