# Channel editor: Dis services and fast AS presentation

[Полный русский контракт](EDITOR-CHANNELS.ru.md) · [Modules](dis/bridge/README.md) ·
[AS](as/README.md) · [Native owner pump](../disVM/OWNER-PUMP.md)

Dis owns documents, revisions/history, editing/selection policy, symbol generations
and service composition. Compiler/debugger/project and virtual filesystem access
belong on that side. AS handles rendering, effects, candidate popups and bounded
lexical colouring near the display engine. Workers receive values, not display
pointers; the UI owner applies only matching revision/window/ticket results.

`Idebridge.serveapp` composes the actual document owner and Dis input controller.
Text insertion, navigation, selection, deletion, Ctrl+A/Z/Y and modifier reset on
focus loss run in Dis. `serve` separately supports typed forwarding to another
Dis input service. Pointer-to-text hit mapping is not yet integrated. Large reply
snapshots use UTF-8-aligned fragments; requests still have a 1024-byte payload bound.
Streaming large-file opening remains a separate unfinished protocol.

`Idecomplete` atomically publishes copied bounded provider records, merge-sorts
its index and binary-searches prefixes, returning at most 64 copied records.
Generation conflicts and mutable alias boundaries are checked. Real compiler/
module declaration providers are not wired yet; fixtures are not source analysis.

The AS lexical worker and presentation adapter are implemented and algorithm-
tested. They do not imply a finished widget layer or worker launch integration.
Current FlashVM TextField formatting is uniform; actual per-range colour rendering
needs the explicit extension described in TEXT-COLOR-EXTENSION.md. Unsupported
renderers must refuse rather than pretend spans were displayed.

Root IDE builds now explicitly select the codec source SDK and emit the document,
codec, input, bridge and completion modules plus debug symbols. Native acceptance:
43 tests on Linux AMD64 with CMake 3.10.2. Six optional application runs pass in
an explicitly matching reference runtime, modes 0/1; the editor checks 36 decoded
frames. This is distinct from running the whole IDE in our unfinished embedded
Dis host, and from physical 64-bit JIT acceptance. The bundled source bootstrap
also compiles all modules, but its ABI must not be assumed compatible with another
runtime. Own Coco/R Limbo/FIR remains the compiler priority. AS parser/Node checks
are not proof of executing the full helpers as SWF/JIT. Node/Python are host-only.
