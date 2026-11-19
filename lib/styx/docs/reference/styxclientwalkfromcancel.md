# styxclientwalkfromcancel

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientwalkfromcancel.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

StyxFid *styxclientwalkfromcancel(StyxFid *base, const char *relative, StyxCancel *cancel, StyxTransfer *result);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `base` | `StyxFid *base` | Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid. |
| `relative` | `const char *relative` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `cancel` | `StyxCancel *cancel` | Explicit sticky cancellation token; one admitted operation per token, with lifetime rules in the public contract. |
| `result` | `StyxTransfer *result` | Copied native acknowledgement: confirmed count, uncertainty and stop reason; not permission to replay an effect. |

## Description

styxclientwalkfromcancel: native client walk operations and lifetime.

### Preserved implementation/header contract

EN/RU: Cancellable logical walk for native PHY clients; owned fid/NULL, required result.
One outer token covers all MAXWELEM fragments and cannot be reused. Result uses
StyxTransfer with count0; success means the complete walk was acknowledged.
Failure cleanup is native detached best-effort clunk, never a caller RPC wait.
Base overload borrows a pinned unopened fid; same confinement as walkfrom.
Отменяемый logical walk; свой fid/NULL, required result, count0; token один на все
fragments. Cleanup без RPC wait caller; base удержан и unopened на время вызова.

## Return value

`StyxFid *`; direct result constants in this body: NULL. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
StyxFid *styxclientwalkfromcancel(StyxFid *base, const char *relative, StyxCancel *cancel, StyxTransfer *result);
```

[Complete calling context](../../client/readable_parts/client-h/part-002.inc#L239)

## See also

- [Implementation: client/walk.c:304](../../client/walk.c#L304)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
