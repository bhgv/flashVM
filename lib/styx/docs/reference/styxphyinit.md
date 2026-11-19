# styxphyinit

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxphyinit.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/server/server.h>

char *
styxphyinit(Styxserver *server, Styxops *ops, RemotePhy *phy, int permissions, int needfile, unsigned queued_frames);
```

## Library

Styx::phy_server; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `server` | `Styxserver *server` | Original server instance and dispatcher-owned state. |
| `ops` | `Styxops *ops` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `phy` | `RemotePhy *phy` | Physical-carrier capability; ownership transfer depends on the constructor/operation. |
| `permissions` | `int permissions` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `needfile` | `int needfile` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `queued_frames` | `unsigned queued_frames` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

styxphyinit: phy/traversal implementation and native state.

### Preserved implementation/header contract

EN/RU: Consume connected reliable ordered stream PHY on every outcome; NULL success/error string.
Fresh zeroed server/borrowed ops; native owner serializes init/wait/process/reply/end.
queued_frames1..128 bounds encoded reply storage; no socket/listener/dial/authentication added.
Link infernostyx_phy instead of the original POSIX backend; original 9P API/codecs remain intact.
Ordinary single-session server: never creates P9MX or a reverse client.
Принять connected reliable ordered PHY всегда; NULL success/error string. Server zeroed,
ops borrowed; один native owner. queued_frames1..128, без socket/listener/dial/new authority.
Обычный server одной сессии; не создаёт P9MX или встречного клиента.

## Return value

`char *`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::phy_server; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
char *error = styxphyinit(&server, &ops, owner->phy, 0555, 1, 2);
```

[Complete calling context](../../examples/memory_phy/server.c#L52)

## See also

- [Implementation: server/phy/traversal.c:145](../../server/phy/traversal.c#L145)
- [Public declaration](../../server/phy/server.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
