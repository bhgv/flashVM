# styxreadreply

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxreadreply.ru.md) · [Index](../index.md)

## Syntax

```c
#include <lib9.h>
#include <styxserver.h>

int styxreadreply(StyxRead *request, void *bytes, ulong count, char *error);
```

## Library

Styx::server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `request` | `StyxRead *request` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `bytes` | `void *bytes` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `count` | `ulong count` | Extent/count used by the selected conversion; byte/rune/record units follow its signature and description. |
| `error` | `char *error` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

styxreadreply: libstyx/read implementation and native state.

### Preserved implementation/header contract

Complete once; borrowed bytes copied during send, nil error means Rread.
Invalid count/buffer leaves request pending and returns0; inactive/staged returns0.
Return1 consumes live transaction/attempts response, not peer acknowledgement.
release(aux) runs once after detachment, before fid/client cleanup; never wait or
reenter dispatch/teardown. It may free the caller request ref. Flush/reset/disconnect
cancel without late replies; fid retirement sends Rerror before its clunk reply.
Завершить один раз; bytes borrowed до возврата.1 attempt,0 inactive/invalid/staged.
release(aux) один раз после detach до fid/client cleanup; без ожидания/reentry.

## Return value

`int`; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::server; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
int styxreadreply(StyxRead *, void *bytes, ulong count, char *error);
```

[Complete calling context](../../vendor/inferno/include/readable_parts/styxserver-h/part-002.inc#L159)

## See also

- [Implementation: vendor/inferno/tools/libstyx/read.c:175](../../vendor/inferno/tools/libstyx/read.c#L175)
- [Public declaration](../../vendor/inferno/include/styxserver.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
