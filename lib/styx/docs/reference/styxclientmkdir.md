# styxclientmkdir

[Русский](styxclientmkdir.ru.md) · [Header](../../client/client.h) · [Source](../../client/open.c)

Link the hosted `Styx::client` target. Pass an unopened directory fid,
a nonempty child name shorter than `STYX_NAME_MAX` bytes, permission bits
within 0777, and transfer result storage. Child names exclude slash, dot
and dot-dot. The peer decides the actual permissions. The function sends
one directory `Tcreate` in read mode and waits up to the request deadline.
It returns one on confirmation. The same owned fid then identifies the
open child directory. Serialize operation and clunk on the fid. An uncertain
reply prevents further use of the fid. Clunk it and inspect the parent.
