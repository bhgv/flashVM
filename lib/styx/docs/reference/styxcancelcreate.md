# styxcancelcreate

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[Русский](styxcancelcreate.ru.md) · [Index](../index.md)

## Syntax

```c
#include <styx/client/client.h>

StyxCancel *styxcancelcreate(void);
```

## Library

Styx::client; hosted native build

## Parameters

| Parameter | Declaration | Meaning and constraints |
| --- | --- | --- |
| — | void | No parameters. |

## Description

styxcancelcreate: native client cancel operations and lifetime.

### Preserved implementation/header contract

EN/RU: Owned sticky cancellation token/NULL; request is nonwaiting and thread-safe.
One admitted operation per token; admitted operation retains it independently.
Caller can free after admission; freeing alone does not request cancellation.
Свой sticky token/NULL; request thread-safe и без ожидания, один operation/token.

## Return value

`StyxCancel *`; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.

## Errors and ownership

Honor the exact contract above, including input/output overlap, capacity, cancellation and disposal rules. A protocol acknowledgement, a native queue result and a completed remote effect are different outcomes. Never infer a permission grant from a notification or automatically replay an uncertain write.

## Availability

Styx::client; hosted native build

## Example

Actual call-site excerpt; its enclosing source provides variable initialization and cleanup.

```c
StyxCancel *child = styxcancelcreate();
```

[Complete calling context](../../client/cancel.c#L243)

## See also

- [Implementation: client/cancel.c:34](../../client/cancel.c#L34)
- [Public declaration](../../client/client.h)
- [API index](../index.md)
- [Protocol and lifetime](../protocol.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
