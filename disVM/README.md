# DisVM: selected Dis interpreter and JIT core

[Broker diagnostics and portable clients](broker/README.md)

[Русский](README.ru.md) · [Upstream manifest](UPSTREAM.json) ·
[Owned tools/update policy](../tools/UPSTREAMS.md) · [Architecture](ARCHITECTURE.md)

Authoritative source: https://github.com/bhgv/Inferno-OS-bhgv, branch **experiment**.
This branch is actively changing. The current snapshot pins
11d959d7af741bf8c7c29c34007bce0a8cd3be37; every imported file has a manifest hash.
Use the explicit update workflow rather than letting builds pull an unknown HEAD.
No unrelated fork supplies this core.

Current v0.1670: [executable selected modules](libraries/README.md),
[fully commented tutorial](examples/standard-modules/README.md),
[native runtime](runtime/README.md). Menu launch 7 runs the actual modules;
module availability and missing /prog/VFS services are documented separately.

[OS filesystem services and example](runtime/FILESYSTEM.md).

## Build and test

```sh
./build-workspace.sh --projects disvm --tests
```

Standalone:

```sh
cmake -S disVM -B build/disvm -DCMAKE_BUILD_TYPE=Release
cmake --build build/disvm --parallel 2
ctest --test-dir build/disvm --output-on-failure
```

This builds **libdisvm_core.a**, the genuine upstream interpreter objects and the
host-selected leaf JIT (amd64, arm64 or riscv64). A POSIX C compiler/CMake is enough
for the archive; Python 3/native C are needed for tests. No native Limbo compiler,
Coco/R, SDL, Tk, graphics SDK, crypto service or whole Inferno installation is
required for this target. The legacy 386 compiler remains reference source.

## Exact current boundary

The archive is **not yet a standalone executable Dis VM**. OS/kernel functions
from kernel.h, scheduler/service integration, module registration and host I/O
must be supplied by a real embedding host. Unresolved host functions are not
filled with pretend-success stubs. Do not link this archive and assume it runs
idecore.dis or supplies an EMU_EXECUTABLE. Native builtin frame/layout migration
and compatible compiler/runtime ABI remain open upstream/port work.

Upstream's new 64-bit JITs are decoded-operation leaf JITs: they preserve original
Dis PCs, keep Module.compiled=0 for this execution ABI and use Module.jit. Some
operations execute native code; allocation, ownership, calls, exceptions and
scheduling still use interpreter/host paths. Retained source does not imply every
operation/backend/platform has been accepted. No canonical new Dis64 ABI is
chosen here, and no merged SWF/Dis container or bytecode format is introduced.

## Selected contents

The manifest includes interpreter/decoder/heap/GC/stack/string/channel/loader/Sys
sources, pointer-map and JIT headers, AMD64/AArch64/RV64 emitters/backends, 386
reference code, required interfaces/declarations, notices and a focused set of
regression drivers/fixtures. Small emu/os boundary files are retained because
upstream channel/error tests extract their actual bodies; they are not compiled
as a whole OS. GUI/security implementations, fonts, applications, archives and
platform build trees are excluded. Some generated builtin declarations remain
in runt.h; they do not link the corresponding GUI/security modules.

The POSIX declaration adapter is in host/include. It isolates the upstream
ARM-only return-address inline from the native host. It is not a complete host
scheduler/FPU implementation. Sources remain pristine; host fixes are not hidden
inside upstream files. Original libinterp NOTICE and distribution legal texts
retain their upstream scope/licences.

## Acceptance scope

The core regressions on Linux AMD64 cover production integer helpers, channel queues and
transfers, Dis parsing/error cleanup, pointer maps, serialization/layout widths,
and actual native AMD64 leaf execution in WORD32 and WORD64 fixture models.
The JIT fixture uses real mmap RW allocation and RX publication and executes
published code directly. Other VM boundaries in these fixtures are injected;
this is not a complete Limbo program, scheduler/GC or Dis/SWF execution test.
With native registers enabled, the current suite has 39 tests, including host/GC
checks and emitter coverage. ARM64/RV64 physical execution is not claimed.

Our Limbo compiler and external module generators remain separate:
[priority](../compiler/limbo/OWN_COMPILER.md),
[one .m for external AS and AS callers of Dis](../compiler/limbo/examples/external-modules/README.md).
The temporary native source bootstrap lives in tools/limbo-bootstrap, not here.

[Selected synchronization 0.1.1 and checks](SYNC.md)

[Embedded host requirements](HOST-REQUIREMENTS.md)

[Shared I/O, native TLS registers and four host ports](host/README.md) · [Module input example](examples/module-image/README.md)

Native host checkpoint v0.1535 embeds active REG in extended up and adds cooperative collector safepoints/root rendezvous; full parallel GC/VM execution remains unfinished. [Contract](host/README.md).

[GC worker contract](host/GC.md) — v0.1536.

[Обновление experiment и готовность к IDE / experiment update and IDE readiness](UPSTREAM-UPDATE.md).

[Value/channel UI bridge](UI-BRIDGE.md) · [Канальный мост и ввод FlashVM](UI-BRIDGE.ru.md).

[Owner pump / Прокачка и сохранение ввода](OWNER-PUMP.ru.md).

[Versioned module namespace / Версии модулей](MODULE-STORE.md).


## Runnable native owner / Исполняемый нативный рантайм

The separate [experimental NC64 runner](runtime/README.md) now loads and executes
matching Limbo modules with production interpreter/JIT, blocking channels and IDE
services. Select `disvm-runtime` or `all` in `build_menu.py`; post-build menu items
4/5 launch Dis and IDE console examples. The core archive above remains a separate
classic SDK. Full GUI, Sys/VFS coverage and parallel shared VM owners remain open.

[Graphical editor v0.1550](../ide/dis/examples/graphical-editor/README.md) adds
an opt-in real NC64 executable, separate from the classic core SDK archive.

## Experiment refresh v0.1555

The selected snapshot is fa3b7abf: guarded short decimal conversions and unique
string stores on AMD64/ARM64, plus backing-span/logical-capacity validation.
The current selection has 66 ordinary core checks and two optional instruction
emulation checks. To enable the latter, supply capstone/unicorn to the selected
Python or set `DVM_EMULATOR_PYTHONPATH` explicitly. This is test tooling only;
physical backend execution and full host/runtime ABI acceptance remain separate.
[Series evidence](../docs/releases/v0.1555.md).


## String-read refresh v0.1558

Current public pin: `61a2942bbe51dd3af507862d7c7081ff5ed8e6cf`, including e6c056c
read/slice/duplicate/concatenation/UTF capacity guards and the upstream native
C/JIT dispatch fixture correction. The selected core now registers 66 ordinary
checks plus three optional instruction-emulation checks; all pass. The added
string-read matrix executes real AMD64/ARM64 instructions rather than treating
foreign emitter compilation as physical execution. [Evidence](../docs/releases/v0.1558.md).


## Real decimals and append v0.1561

Current public pin is `ddbc9f5d6412fe1d4630d7cba00f13502c9b041d`. All 72 selected
core checks have passed, including native/AMD64/ARM64 instruction comparisons.
[Commented executable tutorial](examples/decimal-append/README.md) demonstrates
real conversions, signed zero and retained Unicode string owners on the actual
Dis host. [Series evidence](../docs/releases/v0.1561.md).


## ASCII/wide decimal and clipboard checkpoint v0.1564

Current pin is `1bda41a0e140dabdb76a179c867315b0421df280`. All 75 selected core
checks pass, including the actual retained libmath parser oracle. The embedded
editor now uses [Dis clipboard staging](../ide/dis/CLIPBOARD.md); the source
values, captured range and one-Undo edit stay on Dis. [Series](../docs/releases/v0.1564.md).


## Rune concatenation and Type retain checkpoint v0.1565

Current pin is `5ca377017fe42eac2d95ed0638a5b42ab45ac212`: matching Rune ADDC,
preflight heap/array/loader Type retention and its upstream regressions. The
selected core has 76 passing entries; native runtime/editor and UBSan acceptance
is repeated at this pin. The local memory-parser fixture now renames its retained
entry independently of C whitespace. [Evidence](../docs/releases/v0.1565.md).

[Hosted hybrid big tables / Гибридные таблицы big](examples/hybrid-table/README.md): scalar C core and real Dis interpreter/JIT adapter; language lowering remains pending.


[Native table guards and exception recovery](PAYLOAD_GUARDS.md) ·
[Блокировки таблиц и обработка исключений](PAYLOAD_GUARDS.ru.md).

[Module review and Unicode Regex](MODULE_AUDIT.md): packaged library, confirmed reverse fix and Sys/Debug/Dis boundaries.

[Writing native C modules](runtime/NATIVE_MODULES.md) · [Русский](runtime/NATIVE_MODULES.ru.md) · [Commented tutorial](examples/native-module/README.md).
