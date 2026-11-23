# styxrpc_program_unibus_grant

[Русский](program_unibus_grant.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/program.h>
uint32_t styxrpc_program_unibus_grant(StyxRpcPrograms *programs, const StyxRpcResource *resource);
```

Publish a temporary subtree `.prog/<pid>` on the initialized `programs`
registry. The directory contains `.pid` and `.unibus`. Supply `resource`
with read and write callbacks. The registry copies the descriptor. Keep its
callback context alive through revocation or shutdown.

Returns a unique PID with the high bit set. Use it as a PID route on the
same Styx attachment. Failed validation, insufficient slots or exhausted
identities return zero without publishing. Three registration slots are
required. Revocation releases those slots for future temporary grants.
New grants receive fresh PIDs and Qids, so a stale path or fid cannot reach
a replacement. Retired temporary PIDs cannot be registered manually on the
same session.

Call before serving or from the owning dispatcher. The function does no I/O
and allocates no memory. Link `StyxRpc::serve`.

[Revocation](program_unibus_revoke.md) · [C ↔ DisVM example](../examples/disvm/routed_client.c) · [Source](../src/program_grant.inc)
