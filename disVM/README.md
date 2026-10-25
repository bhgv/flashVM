# DisVM: selected Dis interpreter and JIT core

[Русский](README.ru.md) · [Upstream manifest](UPSTREAM.json) ·
[Owned tools/update policy](../tools/UPSTREAMS.md) · [Architecture](ARCHITECTURE.md)

Authoritative source: https://github.com/bhgv/Inferno-OS-bhgv, branch **experiment**.
This branch is actively changing. The current snapshot pins
96935b070dfb28c18ce9ce35c0bcf7b2b96a63bc; every imported file has a manifest hash.
Use the explicit update workflow rather than letting builds pull an unknown HEAD.
No unrelated fork supplies this core.

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
