# styxclientstat

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientstat.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

int styxclientstat(StyxFid *fid, StyxMeta *output);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `fid` | `StyxFid *fid` | Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid. |
| `output` | `StyxMeta *output` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |

## Description

styxclientstat: native client stat operations and lifetime.

### Preserved implementation/header contract

EN/RU: Query current remote fid metadata;1/0, output changes only on success.
Tstat не открывает путь заново;1/0, output только при успехе.

## Return value

`int`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
good = good && styxclientstat(fid, &metadata) && !strcmp(metadata.name, "status") && metadata.length == 21;
```

[Complete calling context](../../examples/memory_phy/main.c#L32)

## See also

- [Implementation: client/stat.c:130](../../client/stat.c#L130)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
