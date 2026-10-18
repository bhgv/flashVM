# Native architectures / Архитектуры JIT

[Русское руководство](JIT_ARCHITECTURES.ru.md)

v0.348 adds five machine-code generators alongside the existing x86-64 backend.
They share the same AVM1 subset, rooted C execution helpers, instruction budget,
mode selection, cache, interpreter fallback and opt-in exception replay. This is
call-based native code with machine branches; it is not a complete optimizing
AVM1 compiler. Unsupported operations still execute through the interpreter.

В v0.348 добавлены пять генераторов машинного кода. Общими остаются операции,
корни GC, бюджет инструкций, переключение режимов, кеш и диагностический replay.
Полнота AVM1 и платформенная приёмка стадий 8/9 ещё не достигнуты.

| Build target | Implemented calling convention | Instructions / pointer size | Evidence in this workspace |
| --- | --- | --- | --- |
| x86 | Linux i386 System V cdecl | IA-32, 32 bits | Unicorn call/branch/ABI execution |
| x86-64 | Linux System V AMD64 | x86-64, 64 bits | Full host VM tests and examples |
| macOS x64 | Darwin ordinary C, linked RBP | x86-64, 64 bits | Linux instruction/AVM1 pipeline and SDK fixtures; native macOS policy acceptance pending |
| Windows x64 | Microsoft x64 | x86-64, 64 bits | Real Linux ms_abi calls and SDK protocol fixtures; native Windows VM/unwind pending |
| ARM | Linux AAPCS32 | ARMv7 A32, 32 bits; BLX permits Thumb helpers | Unicorn A32 call/branch/ABI execution |
| ARM64 | Linux AAPCS64 | AArch64, 64 bits | Unicorn call/branch/ABI execution |
| RISC-V | Linux standard ILP32-family psABI | RV32I, 32 bits | Unicorn call/branch/ABI execution |
| RISC-V64 | Linux standard LP64-family psABI | RV64I, 64 bits | Unicorn call/branch/ABI execution |

All new encoders emit little-endian instructions. RV32E/RV64E, ARM before v7,
big-endian targets, x32 and AArch64 ILP32 are excluded. Soft/hard-float variants
use their platform C helpers; the emitted ABI passes only a pointer and an
unsigned integer and touches no floating-point registers. RISC-V helper literals
retain the full native pointer width. Native branch veneers avoid the short
conditional-branch ranges on ARM64 and RISC-V. ARM32 and ARM64 direct branches
fit within the compiler's bounded code allocation.

Linux mappings are RW while filled, RX before publication, never RWX. Compiler
instruction-cache synchronization runs before the mapping is returned, including
on ARM/RISC-V. Windows x64 additionally has RW→RX allocation, cache synchronization and registered
unwind metadata (v0.561), documented in [JIT_WINDOWS_X64.md](JIT_WINDOWS_X64.md).
There is no Windows, macOS or Android runtime acceptance claim here; real Windows
execution and the remaining OS ABI/hardened-runtime paths need separate work.
No real foreign-CPU OS build/run was available in this workspace. Emulated
encoder execution is evidence about instructions/ABI, not a full cross-platform
VM run, GC test or performance measurement.

## Build and run the same examples

Architecture selection happens when compiling FlashVM for the host ABI. The
public JIT API does not execute foreign instructions or accept a target argument.
`fvm_jit_backend_name()` reports the selected build ABI; `fvm_jit_available()`
reports whether the backend is compiled in. A later executable-memory denial can
still cause compilation to return UNSUPPORTED and automatic interpreter fallback.

Архитектура выбирается при сборке библиотеки; один и тот же C89 пример работает
на каждой целевой машине. Нельзя запустить ARM-код внутри процесса x86 обычным
вызовом API. Для этого нужны целевая ОС или отдельный эмулятор.

Native Linux build:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
./build/flashvm_examples_embedding_execution_modes_main
./build/flashvm_examples_embedding_jit_debug_main
./build/flashvm_examples_embedding_jit_workers_main
./build/flashvm_examples_embedding_jit_instances_main
```

The execution-modes example first requests interpretation, then automatic JIT,
then demonstrates explicitly enabled exception replay. The lower-level
`flashvm_examples_embedding_jit_main` demonstrates manual compile/attach/detach.
Both retain original bytecode and report unsupported native execution rather
than treating it as successful native compilation.

For cross-builds, install a C/C++ cross toolchain and matching Linux sysroot
**outside this repository**. Example compiler pairs:

| Target | C compiler | C++ compiler | Additional requirement |
| --- | --- | --- | --- |
| x86 | i686-linux-gnu-gcc | i686-linux-gnu-g++ | i386 Linux sysroot; or native multilib GCC with `-m32` |
| ARM | arm-linux-gnueabihf-gcc | arm-linux-gnueabihf-g++ | ARMv7-A CPU flags and matching hard-float sysroot |
| ARM64 | aarch64-linux-gnu-gcc | aarch64-linux-gnu-g++ | LP64 sysroot |
| RISC-V | riscv32-linux-gnu-gcc | riscv32-linux-gnu-g++ | RV32 Linux ILP32 sysroot, not a bare-metal toolchain |
| RISC-V64 | riscv64-linux-gnu-gcc | riscv64-linux-gnu-g++ | RV64 Linux LP64 sysroot |

For example, with the AArch64 compiler/sysroot already configured:

```sh
cmake -S . -B build-arm64 -DCMAKE_SYSTEM_NAME=Linux \
  -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-arm64 -j4
```

Replace the compiler pair and processor with the table's target. These are build
instructions, not claims that those toolchains ran here. Copy/run the executable
and its required shared system libraries on the matching Linux target. Build
third-party dependencies for that same target; do not link host libraries into a
foreign executable. An installed riscv64 compiler can serve RV32 only if its
multilib and Linux sysroot actually include the requested ILP32 ABI.
The DEFS [`INTERPRETER`](../cmake/BuildDefinitions.cmake#L13) selection includes the runtime JIT options; the
DEFS [`JIT`](../cmake/BuildDefinitions.cmake#L13) preset selects automatic compilation/fallback for new contexts. OS worker examples require OS
ports; a minimal NO_OS_PORTS build prints an explicit unavailable message.

## Breakpoints and JIT / Точки останова

Use the existing `flashvm/debug.h` source mapping together with the new
[`fvm_debug_set_breakpoint`](../include/flashvm/debug.h#L51) and [`fvm_debug_set_breakpoint_handler`](../include/flashvm/debug.h#L54) APIs. Offsets are
exact AVM1 action offsets. A source line by itself does not install a breakpoint;
map the line to an actual action offset, then set that offset.

Any installed breakpoint excludes the **whole named function/body** from native
entry, including already cached code and explicit [`fvm_jit_body`](../include/flashvm/jit.h#L24) attachments.
Top-level blocks use the empty name; anonymous C-created AVM1 functions use
`<function>`, matching the existing interpreter debug naming.
All same-named functions share this exclusion: it is deliberately conservative,
not object-identity or source-file disambiguation. Nested functions with different
names can still use JIT. Removing the final breakpoint permits the next entry to
reuse the existing native cache without recompiling it.

При наличии breakpoint вся соответствующая функция или корневой блок
интерпретируется. Внутри неё callback вызывается перед отмеченной инструкцией,
после обновления source/line/offset и проверки бюджета. Возврат [FVM_OK](../include/flashvm/flashvm.h#L42) продолжает
выполнение; другой статус останавливает вызов. При удалении breakpoint следующий
вход снова допускает JIT. Без callback тело всё равно интерпретируется.

Install/change breakpoints on the VM's owning thread, normally while stopped or
before invocation. A currently executing native frame is not rewritten or
restarted: changes affect subsequent entries. The debugger callback runs
synchronously and can wait for host debugger commands on that owning thread;
another OS thread must communicate through host synchronization, not mutate the
context. Callback location pointers are borrowed until return. Do not reenter,
collect, destroy or throw across the C callback. It may edit breakpoints or mode.
The API provides host-controlled stop/resume; it does not invent a GUI debugger
or machine-code single-stepping. Trusted custom native script bridges remain
host callbacks; only JIT attachments have an interpreter body to substitute.

Example: `examples/embedding/jit_debug/main.c` warms the native cache, maps
`compute.as:1`, installs a breakpoint, proves the stopped invocation did not
increment native entries, removes the point and invokes again.

## Threads and multiple VMs / Потоки и несколько VM

Automatic code caches, execution modes, breakpoint tables, logs and counters
belong to each context. There is no global mutable JIT compiler or global mode.
The encoders use invocation-local buffers. Keep each context on one owning
thread; do not concurrently invoke it or mutate its policy/debugger tables.
Manual [FVM_JitCode](../src/jit/internal.hpp#L59) handles have unsynchronized active counts: compile/own one per
VM/thread and never share a handle across concurrent threads. Automatic caches
already follow this rule. Managed objects and function pointers must not be
passed directly to a different context; use graph transport where supported.

`jit_workers/main.c` starts two OS workers before joining either. Each worker
owns its runtime/context and compiles its own function, returning only a number.
The main thread joins even after partial startup failure, then destroys handles.
No worker callback writes shared host state. It demonstrates independent VMs
executing concurrently, not multiple threads inside a single context.

`jit_instances/main.c` keeps two complete runtime/context pairs alive in one
process: one interprets, the other requests JIT. Calls are interleaved on one
thread and statistics remain independent. For concurrency, give each pair its
own owning thread, as the worker example does. Destroy contexts before their
runtimes; context destruction releases its inactive automatic native mappings.

Примеры проверяют ошибки через `if`, освобождают частично созданные ресурсы и
работают также при отключённых assertions. Режимы и breakpoint одной VM не
меняют другую. В многопоточном варианте каждому потоку принадлежит своя VM;
один контекст нельзя одновременно вызывать из нескольких потоков.

## Reproduce instruction-level checks

```sh
cmake --build build --target flashvm_tests_jit_backends
mkdir -p /tmp/flashvm-jit-fixtures
./build/flashvm_tests_jit_backends /tmp/flashvm-jit-fixtures
python -m pip install unicorn
python scripts/check_jit_backends.py /tmp/flashvm-jit-fixtures
```

The optional emulator is a test tool, never a library/runtime dependency.
Synthetic helper addresses are supplied explicitly to the offline encoders;
foreign 32-bit output never truncates live 64-bit host function pointers.
Checks cover arguments (including unsigned 32-bit operands), ABI stack alignment,
callee-saved state, forward/backward native branches and helper error exits for
all five new targets. VM integration and exception/GC/allocation checks run
separately against the actual host backend.

ABI references: [AAPCS32](https://github.com/ARM-software/abi-aa/tree/main/aapcs32),
[AAPCS64](https://github.com/ARM-software/abi-aa/tree/main/aapcs64),
[RISC-V psABI](https://riscv-non-isa.github.io/riscv-elf-psabi-doc/).

## v0.348 validation checkpoint

700/700 Release tests passed on Linux x86-64. 21/21 focused minimal-profile
ASan/UBSan checks passed with heap/global ASan only (stack instrumentation and
LeakSanitizer disabled in this restricted environment). The minimal profile
disables OS workers; actual concurrent worker execution passed in Release.
26 Unicorn cases passed: five targets with normal/far control flow and error
exits, plus ARM Thumb interworking and ARM64/RV64 full-width high addresses.
No benchmark speedup or full foreign-OS execution is inferred from these checks.

## v0.359 repeated encoder execution

Unicorn 2.1.4 reran all 26 current synthetic-address ABI/control-flow cases: x86,
ARM, ARM64, RV32 and RV64; ordinary/far branches, ARM Thumb helper interworking,
64-bit high helper addresses, and successful/error exits. Fixtures were emitted
by the current minimal Release executable, then checked by
`scripts/check_jit_backends.py`. This executes generated instructions with stub
helpers; it remains distinct from running full FlashVM on a foreign CPU/OS.


Windows x64 offline call-plan encoder and actual Linux `ms_abi` validation / автономный генератор и проверка ABI: [JIT_WINDOWS_X64.md](JIT_WINDOWS_X64.md). v0.561 adds Windows x64 selection, W^X allocation and registered unwind metadata; native Windows execution is still unverified. / Добавлены выбор backend, W^X и unwind; нативный Windows запуск ещё не проверен.


macOS Intel offline encoder and linked-frame validation / автономный генератор и цепочка кадров: [JIT_MACOS_X64.md](JIT_MACOS_X64.md). v0.564 adds classic desktop macOS x64 RW→RX publication and explicit signing preparation; native macOS acceptance remains pending / добавлены classic RW→RX и подготовка подписи, нативная приёмка ещё не выполнена.
Offline Darwin ARM64 call encoding is documented in [JIT_MACOS_ARM64.md](JIT_MACOS_ARM64.md). v0.569 adds an opt-in macOS 11.4+ MAP_JIT callback arena; default activation remains OFF pending native acceptance.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_debug_set_breakpoint` | function | [include/flashvm/debug.h:51](../include/flashvm/debug.h#L51) |
| `fvm_debug_set_breakpoint_handler` | function | [include/flashvm/debug.h:54](../include/flashvm/debug.h#L54) |
| `fvm_jit_available` | function | [include/flashvm/jit.h:13](../include/flashvm/jit.h#L13) |
| `fvm_jit_backend_name` | function | [include/flashvm/jit.h:15](../include/flashvm/jit.h#L15) |
| `fvm_jit_body` | function | [include/flashvm/jit.h:24](../include/flashvm/jit.h#L24) |
| `FVM_JitCode` | struct | [src/jit/internal.hpp:59](../src/jit/internal.hpp#L59) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `INTERPRETER` | DEFS switch | [cmake/BuildDefinitions.cmake:13](../cmake/BuildDefinitions.cmake#L13) |
| `JIT` | DEFS switch | [cmake/BuildDefinitions.cmake:13](../cmake/BuildDefinitions.cmake#L13) |
<!-- source-links:end -->
