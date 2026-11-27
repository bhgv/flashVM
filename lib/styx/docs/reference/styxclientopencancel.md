# styxclientopencancel

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientopencancel.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

int styxclientopencancel(StyxFid *fid, int mode, StyxCancel *cancel, StyxTransfer *result);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `fid` | `StyxFid *fid` | Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid. |
| `mode` | `int mode` | Requested operation/permission mode; use the documented protocol constants, not a pathname suffix. |
| `cancel` | `StyxCancel *cancel` | Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract. |
| `result` | `StyxTransfer *result` | Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect. |

## Description

styxclientopencancel: native client open operations and lifetime.

### Preserved implementation/header contract

EN/RU: Cancellable Topen/Tstat require mountfd and result; return1/0, count0.
Original replies win over cancellation. Sent unacknowledged Topen poisons the fid:
no later open/data replay; caller still owns it and may stat/clunk. Tstat leaves
output untouched on every failure and does not poison the fid. Serialize/pin fid.
NULL cancel keeps the normal deadline; non-NULL tokens remain single-use.
Отменяемые Topen/Tstat:1/0, count0, result обязателен; original reply приоритетен.
Topen без acknowledgement блокирует повторный open/data; Tstat не меняет output
при отказе и не poison fid. Caller удерживает/сериализует fid; token single-use.

## Return value

`int`; direct result constants in this body: 0. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
int styxclientopencancel(StyxFid *fid, int mode, StyxCancel *cancel, StyxTransfer *result);
```

[Complete calling context](../../client/readable_parts/client-h/part-002.inc#L309)

## See also

- [Implementation: client/open.c:93](../../client/open.c#L93)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
