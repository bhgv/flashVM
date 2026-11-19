# styxclientrootdirectory

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientrootdirectory.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

int styxclientrootdirectory(StyxClient *client);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `client` | `StyxClient *client` | Native client/session context; retain or consume it exactly as specified by this API. |

## Description

styxclientrootdirectory: native client fid operations and lifetime.

### Preserved implementation/header contract

EN/RU: Copy immutable attached root kind,1 directory/0 file/-1 NULL or auth root.
Caller retains client; no protocol request or mutable server metadata query.
Копировать immutable attach root kind:1 directory/0 file/-1 invalid; без protocol request.

## Return value

`int`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
int styxclientrootdirectory(StyxClient *client);
```

[Complete calling context](../../client/readable_parts/client-h/part-002.inc#L14)

## See also

- [Implementation: client/fid.c:48](../../client/fid.c#L48)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
