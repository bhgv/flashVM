# styxclientkeepalive

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientkeepalive.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

int styxclientkeepalive(StyxClient *client, unsigned idle, unsigned reply, unsigned misses);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `client` | `StyxClient *client` | Native client/session context; retain or consume it exactly as specified by this API. |
| `idle` | `unsigned idle` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `reply` | `unsigned reply` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `misses` | `unsigned misses` | Scalar operand of this operation; protocol/algorithm ranges are described below. |

## Description

styxclientkeepalive: native client fd operations and lifetime.

### Preserved implementation/header contract

EN/RU: Configure carrier-owner heartbeat without protocol wait;1/0. All zeros disables.
idle/reply1..120000ms, misses2..16; NULL/legacy/terminal/invalid refuses unchanged.
One probe retains its tags/deadline through reconfiguration or disabling.
Настроить carrier-owner heartbeat без ожидания;1/0, три нуля отключают будущие probes.

## Return value

`int`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
int styxclientkeepalive(StyxClient *client, unsigned idle, unsigned reply, unsigned misses);
```

[Complete calling context](../../client/readable_parts/client-h/part-001.inc#L316)

## See also

- [Implementation: client/fd.c:128](../../client/fd.c#L128)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
