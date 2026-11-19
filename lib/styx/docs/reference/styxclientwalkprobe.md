# styxclientwalkprobe

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientwalkprobe.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

int styxclientwalkprobe(StyxClient *client, StyxFid *base, const char *relative, unsigned operation, const char *detail);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `client` | `StyxClient *client` | Native client/session context; retain or consume it exactly as specified by this API. |
| `base` | `StyxFid *base` | Pinned remote object/fid; serialize its open/clunk lifetime with callers using that fid. |
| `relative` | `const char *relative` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `operation` | `unsigned operation` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `detail` | `const char *detail` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |

## Description

styxclientwalkprobe: native client traversal operations and lifetime.

### Preserved implementation/header contract

EN/RU: Probe active destination state before crossing a negotiated stream, without pushing it or doing I/O.
Raw profile returns WALK_OK. base/path identify the retained root, operation/detail follow walk.h; WALK_* result.
Проверить active destination до negotiated stream, без push/I/O; raw profile WALK_OK. base/path — retained root.

## Return value

`int`; direct result constants in this body: WALK_INVALID, WALK_OK. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Use the complete layer examples; this lower-level entry is also documented at its exact implementation below.

[Examples](../../examples/README.md)

## See also

- [Implementation: client/traversal.c:153](../../client/traversal.c#L153)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
