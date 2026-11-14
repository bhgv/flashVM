# Resident diagnostic readers

Copyright (c) 2026 bhgv

[Русский](README.ru.md)

`residentdebug.b` implements the public `module/residentdebug.m` interface. It
owns one diagnostic ticket through bounded polling and UTF-8 label assembly,
then attempts cleanup on every exit. Local variables hold capabilities/results;
the library has no shared mutable globals. The captured strings belong to the
caller and remain usable after target retirement.

Build/run instructions: [resident-debug example](../examples/resident-debug/README.md).
Native lifetime, statuses, next-PC semantics and limitations:
[owner location](../runtime/OWNER-LOCATION.md).

Module layout: reusable readers live here, interfaces in module/, native capture
and projections in runtime/diagnostics/, owner publication in resident_threads/,
and application examples in examples/. Each handwritten source stays below500
lines and describes ownership/threading in English and Russian.

Residentstack: bounded owner call chains, [EN](../runtime/OWNER-STACK.md) /
[RU](../runtime/OWNER-STACK.ru.md), with a [runnable example](../examples/resident-stack/README.md).
