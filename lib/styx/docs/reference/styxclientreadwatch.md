# styxclientreadwatch

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientreadwatch.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

void styxclientreadwatch(
    StyxFid *fid, unsigned long long offset, void *bytes, size_t size, StyxCancel *cancel, StyxTransfer *result);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `fid` | `StyxFid *fid` | Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid. |
| `offset` | `unsigned long long offset` | Position in the selected object; file bytes and encoded directory bytes retain their protocol meaning. |
| `bytes` | `void *bytes` | Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views. |
| `size` | `size_t size` | Requested byte extent; the relevant buffer must cover it. |
| `cancel` | `StyxCancel *cancel` | Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract. |
| `result` | `StyxTransfer *result` | Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect. |

## Description

styxclientreadwatch: native client io operations and lifetime.

### Preserved implementation/header contract

EN/RU: Long-lived native read without ordinary request deadline; explicit token required.
Same precision/flush fences as readcancel. Heartbeat/terminal/cancel still operate.
Caller pins/serializes fid and token; requires watchable client, uses one RPC slot.
Долгий native read без обычного deadline; explicit token, heartbeat/cancel сохраняются.

## Return value

No C return value; output/state/ownership effects are specified in Description.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
void styxclientreadwatch(StyxFid *, unsigned long long, void *, size_t, StyxCancel *, StyxTransfer *);
```

[Complete calling context](../../client/readable_parts/client-h/part-003.inc#L62)

## See also

- [Implementation: client/io.c:248](../../client/io.c#L248)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
