# macOS Intel x64 JIT / JIT для macOS Intel x64

## Encoder / Генератор — v0.563

The private [`FVM_JIT_TARGET_MACOS_X64`](../src/jit/backend.hpp#L17) target uses Darwin's ordinary C integer
calling convention: invocation pointer in RDI, unsigned operand in ESI, status
in EAX. Helpers are ordinary fixed-signature C/C++ functions, not Swift, vector,
variadic or aggregate-return callbacks. Full eight-byte function addresses are
resolved explicitly. The target has its own eight-byte pointer-width entry;
Linux System V and Microsoft x64 remain separate selections.

Внутренний target использует обычный Darwin C ABI: состояние в RDI, unsigned
операнд в ESI, статус в EAX. Указатели сохраняются полностью. Swift, varargs,
векторные и сложные возвращаемые типы не используются. Ширина указателя для
этого target явно равна восьми байтам.

The encoder preserves RBP, links it to the parent frame, preserves r12 for the
invocation and reserves an eight-byte alignment slot. Helpers enter with the
normal 16-byte call alignment. Every call reinstalls state and operand after
volatile-register changes. Common status/return paths restore the alignment
slot, r12 and parent RBP. The linked frame helps frame-pointer inspection;
no DWARF/compact-unwind registration or native crash-symbolization completeness
is claimed. VM source maps/debugger ownership retain their existing contracts.

Генератор сохраняет RBP и связывает кадр с родительским, затем сохраняет r12
и выделяет восемь байт для выравнивания. Все выходы восстанавливают стек и оба
регистра. Цепочка frame pointer помогает исследовать стек, но не заменяет
регистрацию DWARF/compact-unwind и не гарантирует полный нативный crash trace.

[`fvm_jit_encode`](../src/jit/backend.hpp#L29) reuses the bounded call-plan driver and rel32 fixups; malformed
branch destinations leave the previous output unchanged. The initial checkpoint
is an offline encoder only: macOS host selection and executable-memory publication
are not enabled by v0.563.

На v0.563 добавлен только offline encoder. Выбор macOS backend и публикация
исполняемой памяти ещё не включены.

## Evidence / Проверки

`tests/jit_macos_x64.cpp` executes generated code on Linux x86-64 GCC with real
C helpers. It checks full-width addresses, high unsigned operands, argument
reinstatement after volatile-register clobbers, helper-entry alignment, saved r12,
linked RBP/return locations, conditional/backward control flow, error returns and
a forward jump over 1 MiB. This is instruction/register-contract evidence on Linux,
not an Apple SDK build, macOS run, Hardened Runtime acceptance or Apple Silicon test.

Проверки исполняют реальные инструкции на Linux, включая цепочку кадров и
выход по ошибке. Они не подтверждают запуск macOS, SDK, подпись приложения,
Hardened Runtime или Apple Silicon.

## Primary references / Первичные источники

- Apple: [64-bit Intel ABI](https://developer.apple.com/documentation/xcode/writing-64-bit-intel-code-for-apple-platforms).
- Apple: [JIT and Apple Silicon memory rules](https://developer.apple.com/documentation/apple-silicon/porting-just-in-time-compilers-to-apple-silicon).

Public code ownership, activation roots, source/machine views and interpreter
fallback: [JIT.md](JIT.md), [JIT_ARCHITECTURES.md](JIT_ARCHITECTURES.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_jit_encode` | function | [src/jit/backend.hpp:29](../src/jit/backend.hpp#L29) |
| `FVM_JIT_TARGET_MACOS_X64` | enum constant | [src/jit/backend.hpp:17](../src/jit/backend.hpp#L17) |
<!-- source-links:end -->
