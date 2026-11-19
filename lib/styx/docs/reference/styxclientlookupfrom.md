# styxclientlookupfrom

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientlookupfrom.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

StyxFid *styxclientlookupfrom(StyxFid *base, const char *path, StyxCancel *cancel, int *missing);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `base` | `StyxFid *base` | Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid. |
| `path` | `const char *path` | Selected object path or stable identity according to the declared type; no implicit authority is granted. |
| `cancel` | `StyxCancel *cancel` | Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract. |
| `missing` | `int *missing` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

styxclientlookupfrom: native client walk operations and lifetime.

### Preserved implementation/header contract

EN/RU: Same lookup from caller-pinned unopened base; serialize its open/clunk through return.
Тот же lookup от pinned unopened base; open/clunk сериализованы до return.

## Return value

`StyxFid *`; direct result constants in this body: NULL. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
StyxFid *styxclientlookupfrom(StyxFid *base, const char *path, StyxCancel *cancel, int *missing);
```

[Complete calling context](../../client/readable_parts/client-h/part-001.inc#L291)

## See also

- [Implementation: client/walk.c:398](../../client/walk.c#L398)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
