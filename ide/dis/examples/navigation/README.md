# Revision-bound navigation example

[Русский](README.ru.md) · [Core build](../../README.md) · [Protocol](../../PROTOCOL.md)

[main.b](main.b) is a fully commented Limbo client of the actual document owner.
It opens `A中\nxyz\n`, maps character offset 4 to line 2/column 1 and back,
finds `yz`, replaces it with `YZ` using the found revision, and rejects a later
query carrying that stale revision. The retained original snapshot stays intact.
The mutation leaves saved=1/revision=2, demonstrating dirty state without pretending
to save a file. Every operation uses typed request/reply channels.

## Build and execute

Use the explicit Limbo compiler, emulator and module headers in the
[reference test instructions](../../README.md#reference-execution-tests).
With BUILD_TESTING and EMU_EXECUTABLE configured, CMake builds navigation.dis
from this source. CTest runs it as `ide_dis_navigation_mode_0` and `_1`.
To run just the example from the workspace root:

```sh
# Build the product plus the example with your already configured external tools.
cmake --build build/ide-dis-test
# Require both successful process exit and the example's PASS marker.
ctest --test-dir build/ide-dis-test -R ide_dis_navigation --output-on-failure
```

Expected output includes `match offset=4 line=2 column=1` and
`PASS idecore navigation 16 checks`. The example uses Sys only for reporting and
for halting its own private reference emulator; the product core imports neither
Sys nor graphics. It is a developer bootstrap, not a deployed UI. Rebuild both
client and product with the same idecore.m; the 0.3.x Reply ABI differs from 0.2.x.
Both reference modes exercise Dis code; they do not prove native JIT entry.

Coordinates count Limbo characters, line is one-based and column zero-based.
Only LF starts a line; CR remains a character. These units cannot be applied
directly to UTF-8-byte compiler diagnostics or UTF-16 editor offsets. There is
no filesystem save, parser, UI widget or execution debugger in this example.
