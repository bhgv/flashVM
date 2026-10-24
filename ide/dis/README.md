# Portable IDE document core

Version 0.3.1. [Русский](README.ru.md).

**Implemented:** a Limbo module compiled to `idecore.dis` with `idecore.sbl`,
providing a single-document state owner through typed channels. This is the first
executable IDE component, not a complete editor or a FlashVM-hosted application.
There is no visual widget, file persistence, compiler/debug service, Dis embedding
or AS bridge in this slice. The product module imports only its own interface;
it uses no Sys, Draw, Tk, Prefab, crypto or native UI API.

[idecore.m](idecore.m) defines the public contract; [idecore.b](idecore.b) implements
`start`, `snapshot`, `handle` and `serve`. See [protocol](PROTOCOL.md) for ordering,
revision, ownership and shutdown rules. Python is optional host test tooling,
not deployed IDE application code.

## Build the actual module

Supply an explicit trusted **native host** Limbo compiler. No toolchain download,
external runtime import or native GUI build occurs through this CMake project.

```sh
cmake -S ide/dis -B build/ide-dis \
  -DLIMBO_EXECUTABLE=/host/bin/limbo -DBUILD_TESTING=OFF
cmake --build build/ide-dis
cmake --install build/ide-dis --prefix release
```

Install contains exactly `share/flashvm/ide/dis/{idecore.dis,idecore.sbl,idecore.m}`.
Tests and the reduced revision fixture are not installed. `start()` is a library
entry point, not a command-line `init`; the eventual host/client loads the module
and calls `start`. The relative PATH is resolved in the caller's resource namespace.
No Python runtime is needed to load/execute this module in a compatible Dis VM.

The build removes stale output before invoking Limbo, requires a successful exit,
nonempty module/debug files, unsigned Dis XMAGIC (`c0 0c 80 30`) and the
exact first two physical symbol lines `limbo .sbl 2.1` / `Idecore`. Each symbol
line must end in LF or CRLF; skipped blank/NUL bytes are not accepted. Only four
module bytes and at most 25 symbol bytes are read for these signature checks.
These are product-admission checks, not a full Dis verifier or atomic release
publication. Failed compilation removes both outputs; keep any previously
published release outside the private build directory.

## Reference execution tests

```sh
cmake -S ide/dis -B build/ide-dis-test \
  -DLIMBO_EXECUTABLE=/host/bin/limbo \
  -DEMU_EXECUTABLE=/host/emu \
  -DLIMBO_MODULE_DIR=/host/inferno/module
cmake --build build/ide-dis-test
ctest --test-dir build/ide-dis-test --output-on-failure
```

The explicitly selected emulator is launched directly with a test `init()` module
and a private build root, without a shell or graphics context. Only test bootstraps
load Sys and use `/dev/sysctl` to halt **their own test emulator**. The product core
never does. A PASS marker and successful process exit are both required; each run
has a timeout. Cases execute in reference modes `-c0` and `-c1`. These modes do not
by themselves prove native JIT entry or target backend coverage.

`idecore_test.b` checks sessions, edits, clean/dirty revisions, Unicode, invalid
operations and stale commands. `limits_test.b` checks the exact 1048576-character
bound, oversize rejection, copied request values, retained replies, backpressure
and competing writers. `revision_test.b` compiles the unchanged production source
with a test-only MaxRevision of 4, exercising terminal revision guards without
billions of edits. That fixture is never installed or distributed as idecore.dis.
`build_guard.py` checks 46 stale/missing/failure/signature cases.
[Admission tutorial and limits](PRODUCT_ADMISSION.md) includes a standalone host check.

Reference bootstrap tools were built from
[InferNode abb1b686](https://github.com/infernode-os/infernode/tree/abb1b6860238f89eb5a10125e561eff4e0a34836)
on Linux AMD64. This is an external development/test toolchain, not the runtime
being imported into FlashVM. The user-selected Dis experiment was rechecked at
[1d8a3e3d](https://github.com/bhgv/Inferno-OS-bhgv/commit/1d8a3e3d3c9694385786b1df784bbd77c7ef9a75).
Execution inside that VM, FlashVM integration and x86-64/arm64/riscv64 native JIT
acceptance remain separate. The new full Coco/R Limbo compilers are still under
development; this module currently uses an explicitly selected bootstrap compiler.

## Deployment format

The IDE application must be interpreted/JIT bytecode: initially Dis for its Limbo
parts, with SWF permitted for AS/FlashVM parts. Host Python tools/native compiler
executables are build aids, not the deployed IDE. Keep these runtime artifacts
separate initially. A combined SWF/Dis container, extra SWF tags and common loader
are deliberately deferred. No new SWF tag or unification format is introduced.

The `.sbl` is emitted by the same build as the module. The existing C symbol-prefix
reader accepts its 230 logical PC mappings in this checkpoint; this does not
validate type/local debug records or implement debugger control. Source filenames
may contain build-machine paths and need source mapping in the eventual host.
Keep source and debug data together; do not silently treat missing source as a
successful debugger integration. [Validation](VALIDATION.md).

## Source navigation

Version 0.3.1 adds revision-bound Locate/Offset queries through the same typed
channel. Rebuild all clients: Reply now includes offset/line/column.
[Coordinate contract](PROTOCOL.md#revision-bound-source-navigation-030).
The reference coordinate regression round-trips all boundaries of empty, Unicode,
LF and CRLF documents and checks stale/closed/range errors in both modes.

[Runnable navigation/search example / Пример навигации и поиска](examples/navigation/README.md).

[0.3.1 compiler/IDE acceptance / Приёмка серии](SERIES_0_3_1.md).
