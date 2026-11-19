# styxclientfiddirectory

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientfiddirectory.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

int styxclientfiddirectory(StyxFid *fid);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `fid` | `StyxFid *fid` | Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid. |

## Description

styxclientfiddirectory: native client fid operations and lifetime.

### Preserved implementation/header contract

EN/RU: Borrow pinned fid; directory1/file0/invalid-1, no RPC.
Заимствовать pinned fid; directory1/file0/invalid-1, без RPC.

## Return value

`int`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
int styxclientfiddirectory(StyxFid *fid);
```

[Complete calling context](../../client/readable_parts/client-h/part-003.inc#L159)

## See also

- [Implementation: client/fid.c:86](../../client/fid.c#L86)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
