# Standalone library maintenance status

Copyright (c) 2026 bhgv

[Русский](MAINTENANCE.ru.md) · [Libraries](README.md) · [Acceptance](VALIDATION.md)

## Completed scope

Styx and libunibus are separate source distributions, implemented in C, with no
DisVM/FlashVM dependency or C++ runtime requirement. Hosted and minimal builds,
relocated SDK consumers, independent offline viewers and bilingual API references
are exercised. Their canonical relationship is checked against 224 Styx and 37
Unibus exported source files; original licenses and provenance remain intact.

The authored example sources have now received a bounded manual contract pass:
all C/header/Python files beneath both `examples/` directories have file purpose
and function explanations. This includes the common catalog server/client,
fixed/TSV/live/HTTP backends, native memory carrier, nested records, native queues,
gateway workers and independent test peers. There are no stock syntax comments in
that reviewed set. `python3 -m lib.tools.example_lines` checks all of it, but a
marker/lexical check is not a proof of semantic completeness.

The [common Styx walkthrough](styx/examples/common.md) explains request flow,
byte offsets, provisional outputs and backend-specific lifetimes. Individual
EN/RU guides cover commands, settings and limits. The TSV recipe is executed
from both README texts during documentation checks. API call-site links are
regenerated and checked against the actual source lines.

## Remaining inherited work

This pass does **not** label every exported implementation comment as manually
reviewed. Original lib9/libstyx and previously annotated codec/client/queue/walk
units still need selective contract review when those units are changed. Some
inherited comments are terse or mechanically phrased; token equivalence and tests
preserve behavior but cannot establish the quality of those explanations.

That maintenance is distinct from source extraction or a missing example. There
is no need to add more demonstration features before returning to DisVM work.
When a concrete defect is found, reproduce it with an affected regression rather
than extending this documentation pass into a new subsystem.

## Deliberate limits

- Hosted execution has been accepted on Linux amd64. Source selections for other
  architectures are not physical-target acceptance.
- Memory PHY demonstrates the interface, not a tested SPI/CAN/radio board driver.
  Reliability/order adaptation remains the physical adapter's responsibility.
- Catalog examples are bounded teaching servers. HTTP/TSV callbacks synchronously
  block their server owner; they are not production asynchronous source schedulers.
- The gateway demonstrates negotiated nested records and client isolation; it does
  not package every engine-broker feature, remote execution or migration facility.
- Mode bits and projected names do not replace provider authentication policy.

These boundaries belong in the respective integration work, not in an indefinite
expansion of the standalone library examples.
