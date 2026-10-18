# Windows x64 JIT / JIT для Windows x64

## Offline encoder / Автономный генератор

The private [`FVM_JIT_TARGET_WINDOWS_X64`](../src/jit/backend.hpp#L16) target is separate from Linux System V
AMD64. It accepts the same decoded call plan and bounded source branches through
[`fvm_jit_encode`](../src/jit/backend.hpp#L30). Its resolver supplies complete eight-byte helper addresses;
no host-pointer truncation or foreign code execution is implicit in encoding.
v0.560 added the encoder and Linux ABI evidence; v0.561 additionally selects
`windows-x64` for ordinary Windows x64 builds and implements native memory/unwind
publication. Full Windows OS execution acceptance is still pending.

Отдельный внутренний target кодирует Microsoft x64 ABI, сохраняя прежний Linux
System V backend. Генерация принимает общий план и resolver полных восьмибайтовых
адресов. В v0.560 добавлены генератор и проверка ABI; v0.561 включает выбор `windows-x64`
для обычных Windows x64 сборок и публикацию кода с unwind-таблицей. Полная
приёмка VM на настоящей Windows ещё не выполнена.

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
at offset 10, complete prologue length 13. Windows activation registers an OS function table for this nonleaf frame.
Merely allocating executable bytes would leave stack walking unreliable.

Фиксированный пролог допускает Windows unwind-описание: сохранение r12
заканчивается на смещении 6, выделение shadow space — на 10, весь пролог — на 13.
Для включения этого нелистового кода под Windows нужна регистрация таблицы
функций ОС; одного разрешения исполнения недостаточно.

The shared frame constants tie encoding and OS metadata to one documented layout:

| Constant / Константа | Contract / Контракт |
| --- | --- |
| [`FVM_JIT_WIN64_SHADOW_BYTES`](../ports/windows/code.c#L26) | 32 bytes of caller-owned home slots / 32 байта shadow space |
| [`FVM_JIT_WIN64_PUSH_END`](../ports/windows/code.c#L35) | Saved r12 becomes unwind-visible at offset 6 / r12 сохранён на смещении 6 |
| [`FVM_JIT_WIN64_ALLOC_END`](../ports/windows/code.c#L34) | Allocation completed at offset 10 / выделение закончено на смещении 10 |
| [`FVM_JIT_WIN64_PROLOGUE_BYTES`](../src/jit/windows_x64_constants.h#L14) | Fixed prologue length 13 / длина пролога 13 |
| [`FVM_JIT_WIN64_UNWIND_CODES`](../ports/windows/code.c#L33) | Two records describing allocation and push / два unwind-кода |

## Availability correction / Исправление доступности — v0.562

The v0.561 pointer-width switch omitted the Windows target. Selection and offline
encoding existed, but [`fvm_jit_available`](../include/flashvm/jit.h#L13) would reject Windows because its ABI
width was reported as zero. v0.562 explicitly returns eight bytes for Microsoft
x64 (LLP64); this corrects the earlier activation claim. No actual Windows run
is retroactively claimed. New target-width and macro-specialized production
selection tests cover both the selected target and its linked width lookup.

В v0.561 выбор Windows backend был добавлен, но switch ширины указателя пропускал
Windows target. Поэтому проверка доступности возвращала false. v0.562 исправляет
ширину на восемь байт; прежнее утверждение о включении уточняется этим дефектом.
Добавлены проверки всех ABI и настоящего кода выбора под Windows-макросами.
Это не означает, что выполнена нативная Windows приёмка.

## Windows publication and ownership / Публикация и владение

[`fvm_jit_host_target`](../src/jit/backend.cpp#L6) selects Microsoft x64 only for `_WIN32` with an ordinary
64-bit x86 ABI; 32-bit Windows, ARM64/ARM64EC and other unimplemented Windows ABIs
remain unsupported. [`fvm_jit_backend_name`](../include/flashvm/jit.h#L15) reports `windows-x64`. Existing compile,
function attachment, source/machine views, active-frame lifetime guards and
interpreter fallback contracts remain unchanged. Availability reports compiled
support; OS policy can still reject an individual mapping as UNSUPPORTED.

Включён только обычный Windows x64 ABI; Windows x86, ARM64/ARM64EC и прочие
неподдержанные ABI остаются выключенными. Имя backend — `windows-x64`. Прежние
правила compile/attach/активных вызовов, source map и fallback сохраняются.
Наличие backend не гарантирует разрешения каждой публикации политикой ОС.

The [Windows adapter](../ports/windows/code.c) validates the exact encoder prologue
and epilogue before allocation. `VirtualAlloc` reserves/commits PAGE_READWRITE
storage for code, DWORD-aligned eight-byte UNWIND_INFO and one RUNTIME_FUNCTION.
It copies source bytes and zeroes alignment padding while writable. The entry
covers code only, with image-relative 32-bit start/end/unwind offsets. The fixed
unwind codes describe UWOP_ALLOC_SMALL (32 bytes) and UWOP_PUSH_NONVOL (r12), in
reverse prologue order. Code, metadata and function table are then sealed with
`VirtualProtect` to PAGE_EXECUTE_READ, followed by `FlushInstructionCache` and
`RtlAddFunctionTable`. Outputs publish only after registration succeeds. No RWX
mapping or remaining writable alias is created.

Переходник проверяет точный пролог/эпилог, выделяет RW-область, копирует код и
формирует выровненные UNWIND_INFO/RUNTIME_FUNCTION с относительными смещениями.
Затем устанавливает RX, синхронизирует кеш инструкций и регистрирует таблицу ОС.
Выходы публикуются только после успеха всех шагов; RWX и writable alias нет.

The internal `fvm_port_code_create(bytes, size, out, mapped)` preserves both
outputs on failure: invalid/null arguments or mismatched frame return
INVALID_ARGUMENT, native allocation failures with ERROR_NOT_ENOUGH_MEMORY or
ERROR_OUTOFMEMORY return OUT_OF_MEMORY, other allocation/protection/cache/table
failures return UNSUPPORTED. Code larger than the existing JIT bound is rejected
before copying. Mapping cleanup follows every failed publication. The private
`mapped` value is an opaque release cookie (Windows table offset, Linux mapping
length); it is **not** the public emitted byte count. [`fvm_jit_machine_code`](../include/flashvm/jit.h#L33) and
[`fvm_jit_code_size`](../include/flashvm/jit.h#L35) continue to exclude alignment/unwind/table/page overhead.

При ошибке оба выхода внутреннего create неизменны: неверные аргументы/пролог —
INVALID_ARGUMENT, известные ошибки нехватки памяти — OUT_OF_MEMORY, запрет
выделения/защиты/кеша/регистрации — UNSUPPORTED. Лимит размера проверяется до
копирования. Внутренний mapped служит cookie очистки, а не размером машинного
кода; публичные размеры не включают unwind-таблицу, выравнивание и страницы ОС.

[`fvm_port_code_release`](../ports/code_port.c#L75) unregisters using `RtlDeleteFunctionTable` **before**
`VirtualFree(MEM_RELEASE)`. A null/inactive mapping needs no work. Callers must
ensure no active frame, debugger stack walk or other thread is using the mapping;
existing host ownership rules do not permit release races. If OS unregistration
unexpectedly rejects a previously registered table, the adapter conservatively
retains the mapping rather than freeing OS-visible table storage. Native metadata
is not managed by the VM GC. Each nested compiled mapping registers its own frame.

Перед освобождением страницы таблица снимается через RtlDeleteFunctionTable.
Host обязан исключить активное исполнение, одновременный stack walk и гонку с
освобождением. При неожиданном отказе снятия таблицы память сохраняется, чтобы
ОС не получила висячий указатель. Метаданные нативные, не корни GC; каждый
вложенный скомпилированный блок имеет свою таблицу.

## Build / Сборка

On a Windows x64 development machine with C/C++ compiler, Windows SDK and CMake,
use the existing SDK-free minimal profile (no FFmpeg or graphics package needed):

```sh
cmake -S . -B build-win64 -DFVM_DEFS_FILE=cmake/profiles/minimal-jit.defs -DCMAKE_BUILD_TYPE=Release
cmake --build build-win64 --config Release
ctest --test-dir build-win64 -C Release --output-on-failure -R "jit_windows_unwind|jit_basic|jit_control|jit_numeric"
```

Это команды для нативной Windows x64 машины с компилятором и Windows SDK;
они здесь не выполнены. SDK-free означает отсутствие дополнительных мультимедиа
SDK; сам Windows SDK необходим. MSVC/multi-config используют `--config Release`.
`tests/jit_windows_unwind.cpp` is a Windows-only acceptance fixture: it queries the
OS table with `RtlLookupFunctionEntry`, virtually unwinds synthetic frames with
`RtlVirtualUnwind` at prologue offsets 0/4/6/10/13, checks r12/RSP/return recovery
and checks table removal on destroy. Its body is omitted on Linux; since v0.564 CTest explicitly reports it as Skipped
there. Earlier Linux passes for an empty body were not Windows unwind evidence.

## Validation / Проверки

`tests/jit_windows_x64.cpp` runs emitted bytes on Linux x86-64 GCC using real
`ms_abi` entry/helper functions and the existing RW→RX Linux mapping. It checks:

- full-width helper addresses and high unsigned operands;
- argument reinstatement after volatile-register clobbers;
- helper-entry stack alignment and writes to all four mandatory home slots;
- repeated conditional/backward jumps, status exit and a forward jump over 1 MiB;
- unchanged encoding output on invalid branch targets.

`tests/jit_windows_code_port.cpp` executes the actual Windows C adapter against
a controlled SDK boundary: protection/cache/registration order, exact unwind
bytes/relative offsets, allocation/protection/cache/table failures, unchanged
outputs, cleanup, all four metadata alignment cases using actual encoder output, and
unregister-before-free including a rejected unregister.
The Windows production C branch also compiles as strict C89 against controlled
SDK declarations; this does not verify real Windows SDK linkage.

Эти проверки исполняют реальные инструкции Microsoft ABI на Linux; это не запуск
Windows, не проверка Windows SEH/stack walk и не полная AVM1/GC приёмка на другой ОС.
На остальных host конфигурациях Linux-only часть теста не исполняется.

## Primary references / Нормативные источники

- Microsoft: [x64 calling convention](https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention).
- Microsoft: [x64 stack usage](https://learn.microsoft.com/en-us/cpp/build/stack-usage).
- Microsoft: [VirtualProtect and instruction-cache responsibility](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualprotect).
- Microsoft: [RtlAddFunctionTable](https://learn.microsoft.com/en-us/windows/win32/api/winnt/nf-winnt-rtladdfunctiontable).
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
| `fvm_jit_available` | function | [include/flashvm/jit.h:13](../include/flashvm/jit.h#L13) |
| `fvm_jit_backend_name` | function | [include/flashvm/jit.h:15](../include/flashvm/jit.h#L15) |
| `fvm_jit_code_size` | function | [include/flashvm/jit.h:35](../include/flashvm/jit.h#L35) |
| `fvm_jit_encode` | function | [src/jit/backend.hpp:30](../src/jit/backend.hpp#L30) |
| `fvm_jit_host_target` | function | [src/jit/backend.cpp:6](../src/jit/backend.cpp#L6) |
| `fvm_jit_machine_code` | function | [include/flashvm/jit.h:33](../include/flashvm/jit.h#L33) |
| `FVM_JIT_TARGET_WINDOWS_X64` | enum constant | [src/jit/backend.hpp:16](../src/jit/backend.hpp#L16) |
| `FVM_JIT_WIN64_ALLOC_END` | enum constant | [ports/windows/code.c:34](../ports/windows/code.c#L34) |
| `FVM_JIT_WIN64_PROLOGUE_BYTES` | macro / define | [src/jit/windows_x64_constants.h:14](../src/jit/windows_x64_constants.h#L14) |
| `FVM_JIT_WIN64_PUSH_END` | enum constant | [ports/windows/code.c:35](../ports/windows/code.c#L35) |
| `FVM_JIT_WIN64_SHADOW_BYTES` | enum constant | [ports/windows/code.c:26](../ports/windows/code.c#L26) |
| `FVM_JIT_WIN64_UNWIND_CODES` | enum constant | [ports/windows/code.c:33](../ports/windows/code.c#L33) |
| `fvm_port_code_create` | function | [ports/code_port.c:32](../ports/code_port.c#L32) |
| `fvm_port_code_release` | function | [ports/code_port.c:75](../ports/code_port.c#L75) |
<!-- source-links:end -->
