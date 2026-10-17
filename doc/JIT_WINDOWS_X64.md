# Windows x64 JIT / JIT для Windows x64

## Offline encoder / Автономный генератор

The private [`FVM_JIT_TARGET_WINDOWS_X64`](../src/jit/backend.hpp#L16) target is separate from Linux System V
AMD64. It accepts the same decoded call plan and bounded source branches through
[`fvm_jit_encode`](../src/jit/backend.hpp#L28). Its resolver supplies complete eight-byte helper addresses;
no host-pointer truncation or foreign code execution is implicit in encoding.
This checkpoint adds the encoder and ABI execution evidence. It does **not**
yet select this backend on Windows or enable Windows executable-memory allocation.

Отдельный внутренний target кодирует Microsoft x64 ABI, сохраняя прежний Linux
System V backend. Генерация принимает общий план и resolver полных восьмибайтовых
адресов. На этой контрольной точке Windows backend ещё не выбирается при запуске:
добавлены генератор и проверка ABI, а не приёмка всей VM под Windows.

The machine entry receives an invocation pointer in RCX, preserves r12, moves the
pointer to r12 and reserves the mandatory 32-byte helper shadow area. Each helper
receives state in RCX and an unsigned 32-bit operand in EDX; its status returns in
EAX. Calls reinstall both arguments after volatile-register clobbers. Push r12
plus the fixed allocation maintain 16-byte alignment before every helper call.
The common epilogue restores the stack and r12 on successful, conditional and
error exits. Native rel32 jumps implement forward/backward flow without an
interpreter dispatch loop. ENDBR64 remains at the indirect entry.

Точка входа получает состояние в RCX; r12 сохраняется и удерживает состояние.
Перед каждым helper зарезервированы четыре home slots (32 байта), стек выровнен
на 16 байт. Операнд передаётся в EDX, результат — в EAX; аргументы заново
устанавливаются после каждого вызова. Общий эпилог восстанавливает стек и r12,
в том числе при ошибке. Переходы rel32 остаются машинными переходами.

The fixed prologue and constants in `src/jit/windows_x64_constants.h` describe
an unwindable frame: saved r12 at prologue offset 6, 32-byte allocation completed
at offset 10, complete prologue length 13. Windows activation additionally needs
an OS function table for this nonleaf frame; merely allocating executable bytes
would leave stack walking unreliable.

Фиксированный пролог допускает Windows unwind-описание: сохранение r12
заканчивается на смещении 6, выделение shadow space — на 10, весь пролог — на 13.
Для включения этого нелистового кода под Windows нужна регистрация таблицы
функций ОС; одного разрешения исполнения недостаточно.

## Validation / Проверки

`tests/jit_windows_x64.cpp` runs emitted bytes on Linux x86-64 GCC using real
`ms_abi` entry/helper functions and the existing RW→RX Linux mapping. It checks:

- full-width helper addresses and high unsigned operands;
- argument reinstatement after volatile-register clobbers;
- helper-entry stack alignment and writes to all four mandatory home slots;
- repeated conditional/backward jumps, status exit and a forward jump over 1 MiB;
- unchanged encoding output on invalid branch targets.

Эти проверки исполняют реальные инструкции Microsoft ABI на Linux; это не запуск
Windows, не проверка Windows SEH/stack walk и не полная AVM1/GC приёмка на другой ОС.
На остальных host конфигурациях Linux-only часть теста не исполняется.

## Primary references / Нормативные источники

- Microsoft: [x64 calling convention](https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention).
- Microsoft: [x64 stack usage](https://learn.microsoft.com/en-us/cpp/build/stack-usage).
- Microsoft: [x64 exception handling and dynamic unwind tables](https://learn.microsoft.com/en-us/cpp/build/exception-handling-x64).

Public compilation, ownership, source/machine views and callback/GC restrictions
remain in [JIT.md](JIT.md) and [JIT_ARCHITECTURES.md](JIT_ARCHITECTURES.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_jit_encode` | function | [src/jit/backend.hpp:28](../src/jit/backend.hpp#L28) |
| `FVM_JIT_TARGET_WINDOWS_X64` | enum constant | [src/jit/backend.hpp:16](../src/jit/backend.hpp#L16) |
<!-- source-links:end -->
