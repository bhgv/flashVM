# styxclientwalk

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientwalk.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

StyxFid *styxclientwalk(StyxClient *client, const char *relative);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `client` | `StyxClient *client` | Native client/session context; retain or consume it exactly as specified by this API. |
| `relative` | `const char *relative` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

styxclientwalk: native client walk operations and lifetime.

### Preserved implementation/header contract

EN/RU: Walk canonical relative path, dot for root; owned independent fid/NULL.
Reject absolute/empty/dotdot/embedded dot components before wire I/O. Multiple
MAXWELEM batches preserve the same new fid. Calls serialized by connection mutex.
Пройти canonical relative path; свой fid/NULL, без выхода выше root.

## Return value

`StyxFid *`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
StyxFid *fid = styxclientwalk(client, argv[2]);
```

[Complete calling context](../../examples/client.c#L56)

## See also

- [Implementation: client/walk.c:226](../../client/walk.c#L226)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
