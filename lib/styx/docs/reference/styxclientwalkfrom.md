# styxclientwalkfrom

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientwalkfrom.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

StyxFid *styxclientwalkfrom(StyxFid *base, const char *relative);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `base` | `StyxFid *base` | Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid. |
| `relative` | `const char *relative` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

styxclientwalkfrom: native client walk operations and lifetime.

### Preserved implementation/header contract

EN/RU: Walk from a borrowed pinned unopened fid; dot clones, owned independent result/NULL.
Serialize base open/clunk against this call; concurrent walks are allowed. File descendants
and unsafe paths refuse before wire effects. No fallback to the original attach pathname.
Пройти от pinned unopened fid; dot клонирует, свой result/NULL; без fallback к attach pathname.

## Return value

`StyxFid *`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
StyxFid *styxclientwalkfrom(StyxFid *base, const char *relative);
```

[Complete calling context](../../client/readable_parts/client-h/part-002.inc#L192)

## See also

- [Implementation: client/walk.c:254](../../client/walk.c#L254)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
