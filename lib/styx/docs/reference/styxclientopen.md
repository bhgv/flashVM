# styxclientopen

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientopen.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

int styxclientopen(StyxFid *fid, int mode);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `fid` | `StyxFid *fid` | Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid. |
| `mode` | `int mode` | Requested operation/permission mode; use the documented protocol constants, not a pathname suffix. |

## Description

styxclientopen: native client open operations and lifetime.

### Preserved implementation/header contract

EN/RU: Open unopened fid in mode0/1/2 without create/truncate;1/0.
Caller serializes a fid's calls. Ropen iounit limits every later chunk.
An unacknowledged sent open fences later open/data calls; stat/clunk remain allowed.
Открыть fid без create/truncate;1/0, iounit ограничивает chunk. Sent open без
acknowledgement запрещает повторный open/data; stat/clunk остаются доступны.

## Return value

`int`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
int valid = fid && styxclientopen(fid, 0); /* EN: Declare operation-local storage; its initializer specifies the initial value. RU: Объявить память операции; initializer задаёт начальное значение. */
```

[Complete calling context](../../examples/client.c#L61)

## See also

- [Implementation: client/open.c:79](../../client/open.c#L79)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
