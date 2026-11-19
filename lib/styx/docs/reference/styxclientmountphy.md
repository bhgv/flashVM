# styxclientmountphy

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientmountphy.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

StyxClient *
styxclientmountphy(RemotePhy *phy, unsigned msize, const char *user, const char *name, unsigned timeout, unsigned limit);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `phy` | `RemotePhy *phy` | Physical-carrier capability; ownership transfer depends on the constructor/operation. |
| `msize` | `unsigned msize` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `user` | `const char *user` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `name` | `const char *name` | Component or schema name in this operation; the function validates its own encoding/length limits. |
| `timeout` | `unsigned timeout` | Native wait budget in milliseconds; zero and upper-bound behavior follow the function contract. |
| `limit` | `unsigned limit` | Explicit local quota; permitted ranges and wire/body units are given in the description. |

## Description

styxclientmountphy: native client phy operations and lifetime.

### Preserved implementation/header contract

EN/RU: Consume owned connected reliable ordered byte-stream PHY on every outcome; owned client/NULL.
Uses the same native multiplex/heartbeat/cancel engine as mountfd, with no fd requirement.
Ordinary single 9P session: limit bounds outstanding RPCs, never enables P9MX/reverse sessions.
Driver callbacks remain native-only; stream adaptations must precede this admission boundary.
Принять connected reliable ordered stream PHY всегда; свой client/NULL, общий native engine,
без fd requirement; limit ограничивает RPC одной сессии, не включает P9MX.
Packet/reliability adaptations должны предшествовать этому уровню.

## Return value

`StyxClient *`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
client = styxclientmountphy(client_phy, 256, "demo", "", 3000, 4);
```

[Complete calling context](../../examples/memory_phy/main.c#L113)

## See also

- [Implementation: client/phy.c:32](../../client/phy.c#L32)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
