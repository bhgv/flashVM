# styxclientmount

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxclientmount.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

StyxClient *styxclientmount(const StyxTransport *transport, unsigned msize, const char *user, const char *name);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| `transport` | `const StyxTransport *transport` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `msize` | `unsigned msize` | Scalar operand of this operation; protocol/algorithm ranges are described below. |
| `user` | `const char *user` | Referenced value of the declared type; lifetime and input/output effects remain those of the implementation. |
| `name` | `const char *name` | Component or schema name in this operation; the function validates its own encoding/length limits. |

## Description

styxclientmount: native client session operations and lifetime.

### Preserved implementation/header contract

EN/RU: Consume a valid transport on every outcome; negotiate256..65536 bytes and
attach NOFID with copied host-supplied identity hints. Invalid callback struct
is not adopted. Caller serializes creation; returned connection is owned/NULL.
Принять valid transport всегда; negotiate+attach, свой client/NULL.

## Return value

`StyxClient *`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
StyxClient *styxclientmount(const StyxTransport *transport, unsigned msize, const char *user, const char *name);
```

[Complete calling context](../../client/readable_parts/client-h/part-001.inc#L53)

## See also

- [Implementation: client/readable_parts/session-c/part-002.inc:120](../../client/readable_parts/session-c/part-002.inc#L120)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
