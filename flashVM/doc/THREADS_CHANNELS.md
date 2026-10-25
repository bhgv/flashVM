> Current status: [ActionScript API](ACTIONSCRIPT_EXTENSIONS.md) and [ownership](CONCURRENCY_SAFETY.md). The text below is an earlier-stage design document; its limitations do not describe the current API.

# Thread and channel API design (NOT implemented)

This document describes proposed ActionScript extensions, not existing runtime features. The only implemented feature in this step is [`fvm_meta_call`](../include/flashvm/meta_call.h#L22), which calls a native `__call` handler with `(owner, defining_metatable, indexed_argument_table)`. User arguments start at index zero.

Proposed thread API: `thread.run`, `thread.run_suspended`, `thread.join`, `thread.sleep`, `thread.sleepms`, `thread.sleepus`. A thread handle would expose `start`, cooperative `stop`, `state`, and `ret`. Each worker must have its own VM context. Data is deep-copied on transfer, except explicitly shared channel handles. Non-transferable native userdata and closures must be rejected unless a transfer policy exists.

Proposed channel syntax: `channel <- value`, `value <- channel`, `ready ?- channel`. Internal channels preserve typed messages. TCP and UDP channels require serialization, framing (TCP), bounds checking, and explicit network errors. TLS requires certificate and hostname verification. A readiness probe cannot guarantee a subsequent receive will not block.

Proposed endian annotations: `::d8/16/32/64` for big endian and `::8/16/32/64d` for little endian. Numeric overflow, signedness, floating-point format, and string encoding must be specified.

The syntax for channels, extended `for` loops and `<...>` literals requires a separate extended ActionScript compiler; it cannot be implemented solely by adding AVM1 opcodes. Platform adapters for OS threads and sockets exist in `ports/`, but do not yet implement these language-level facilities.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_meta_call` | function | [include/flashvm/meta_call.h:22](../include/flashvm/meta_call.h#L22) |
<!-- source-links:end -->
