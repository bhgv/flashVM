# styxclientcancellable

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientcancellable.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

int styxclientcancellable(StyxClient *client);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `client` | `StyxClient *client` | Native client/session context; retain or consume it exactly as specified by this API. |

## Description

styxclientcancellable: native client fid operations and lifetime.

### Preserved implementation/header contract

EN/RU: Immutable explicit-cancel capability,1 for carrier-owner,0 for NULL/legacy; no I/O.
Immutable explicit-cancel capability:1 carrier-owner,0 NULL/legacy; без I/O/прав.

## Return value

`int`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
int styxclientcancellable(StyxClient *client);
```

[Complete calling context](../../client/readable_parts/client-h/part-003.inc#L16)

## See also

- [Implementation: client/fid.c:67](../../client/fid.c#L67)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
