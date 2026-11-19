# styxdeferread

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxdeferread.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>
#include <styxserver.h>

StyxRead *styxdeferread(Styxserver *server, void *aux, void (*release)(void *));
```

## Library

Styx::server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `server` | `Styxserver *server` | Original server instance and dispatcher-owned state. |
| `aux` | `void *aux` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `void (*release)(void *)` | `void (*release)(void *)` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

styxdeferread: libstyx/read implementation and native state.

### Preserved implementation/header contract

Native owner-thread byte-read extension, no new wire messages. Defer only during
read callback, once per non-directory/nonzero read and at most one per fid.
Set *n=0 and return nil on success. Returned request is caller-owned; NULL
leaves aux caller-owned. At most128 pending reads per client. No buffers retained.
Worker threads pass copied data through native mailboxes/fds, never request pointers.
Native byte-read extension; только read callback, *n=0/nil при успехе, свой request.
Worker передаёт copied data через mailbox/fd; request доступен только owner-thread.

## Return value

`StyxRead *`; direct result constants in this body: nil. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::server; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
StyxRead *styxdeferread(Styxserver *, void *aux, void (*release)(void *));
```

[Complete calling context](../../vendor/inferno/include/readable_parts/styxserver-h/part-002.inc#L75)

## See also

- [Implementation: vendor/inferno/tools/libstyx/read.c:100](../../vendor/inferno/tools/libstyx/read.c#L100)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
