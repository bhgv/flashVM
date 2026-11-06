# Linux runtime architectures / Архитектуры Linux

The native Linux runner selects a backend from the processor reported by CMake,
then verifies the C compiler target. Both console and graphical executables use
that selection. `disvm --version` names the selected backend. No architecture is
silently substituted with another instruction set.

| Linux userspace | CMake processor names | Backend | Current scope |
| --- | --- | --- | --- |
| x86-64 | x86_64, amd64 (case insensitive) | comp-amd64.c | NC64 console and IDE |
| AArch64 | aarch64, arm64 | comp-arm64.c | NC64 console and IDE build selection |
| RISC-V 64 | riscv64, rv64 | comp-riscv64.c | NC64 console and IDE build selection |
| x86 32-bit | i386, i686 | not connected to this runner | requires a separate 32-bit host/ABI port |
| ARM 32-bit | arm, armv7l | not connected to this runner | requires a separate 32-bit host/ABI port |

## Build on the target machine

```sh
git pull --ff-only
python3 build_menu.py
```

Select the normal runtime or complete workspace build. On ARM64 the configuration
prints `DisVM Linux64 runtime backend: arm64`; on RV64 it prints `riscv64`.
No cross compiler or QEMU is required when building on that machine. A 64-bit CPU
with a 32-bit userspace is not an NC64 host: the compiler must also use 64-bit
pointers. Existing generated modules must match the shipped NC64 compiler.

На снимке ARM64-сборка останавливалась из-за проверки, допускавшей только x86-64,
и жёсткого подключения comp-amd64.c. Исправлены оба места. Выбор backend общий
для консольного и графического исполняемых файлов; компилятор неправильной
архитектуры теперь отклоняется при конфигурации с понятной причиной.

## Verification boundary

The v0.1697 console runtime was cross-compiled into actual AArch64 and RV64 ELF
executables. Under QEMU user emulation, the channel example, vector model,
vector pane owner, desktop owner and plain/optimized array-alias regressions run
with both `--jit` and `--no-jit`. The tests require positive native-leaf counters
for JIT and zero for interpretation. Host-generated NC64 module/header layouts
are shared by these LP64 targets; the target C objects and leaf emitter are
cross-compiled, never borrowed AMD64 objects. This is console execution under
emulation, not physical-board, SDL-device or complete graphical-port acceptance.
Normal x86-64 headless IDE tests remain separate evidence.

## Why x86-32 and ARM32 need more work

The current runner explicitly checks eight-byte WORDs, pointers and cells plus
NC64 Sys/Alt frame offsets. The imported x86 compiler backend and Inferno's ARM
backend use the classic 32-bit runtime contract, with a different native-PC JIT
execution model. Merely removing the pointer check or selecting those files would
not produce a correct runnable port.

A 32-bit port must select the matching four-byte bootstrap/module format, generate
and validate all host-call frames, adapt table/captured-result/relative-index
boundaries, connect native-PC dispatch and executable-page/cache handling, and
run the same ownership/GC/editor tests on each target. Until then these builds
fail explicitly. No new common Dis64 wire or calling ABI is assigned by the Linux
backend selector; NC64 remains a private experimental target.

## Dedicated-spawn console regression

After building the target console, run the forty-guest arithmetic and channel
fixture using an NC64 bootstrap compiler executable for the test host:

```sh
python3 disVM/runtime/test_module_continuation.py TARGET_DISVM HOST_LIMBO disVM/upstream/module \
    --portable-overlap --runner qemu-aarch64 -L /usr/aarch64-linux-gnu
```

For RV64 use `qemu-riscv64` and the corresponding RV64 sysroot. On a physical
target omit `--runner`. The portable check uses interpretation because the
guarded JIT profile is currently restricted to AMD64. It checks guest start
and retirement, channel completions, scalar trace counters, and arithmetic
results. The x64 private test additionally uses controlled overlapping
admissions and runs both the interpreter and JIT.

The fixture also exercises guarded MOVP with native GC. Forty guests
publish and retain arrays through a shared module slot. The parent clears
the slot before allowing the guests to check their retained arrays again.
The test requires a successful shared MOVP involving module data.

The portable regression selects the dedicated profile with `--native-spawn`.
On AMD64 and in interpreter mode on ARM64/RV64, this enables the existing
guarded scalar, pointer, mapped-copy and channel paths together. Supplying
guarded/parallel switches explicitly selects the individual paths. Non-AMD64
JIT retains its existing profile without guarded copies.

Native Linux builds with runtime tests enabled register this check as
`disvm_dedicated_spawn_portable` on all three console backends. Run it
with `ctest --test-dir BUILD_DIR -R '^disvm_dedicated_spawn_portable$' --output-on-failure`.
It checks forty distinct live OS thread identities, balanced dispatch,
and collector progress and shutdown.
