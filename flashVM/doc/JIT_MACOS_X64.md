# macOS Intel x64 JIT / JIT для macOS Intel x64

## Encoder / Генератор — v0.563

The private [`FVM_JIT_TARGET_MACOS_X64`](../src/jit/backend.hpp#L19) target uses Darwin's ordinary C integer
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

[`fvm_jit_encode`](../src/jit/backend.hpp#L38) reuses the bounded call-plan driver and rel32 fixups; malformed
branch destinations leave the previous output unchanged. v0.563 supplied the offline encoder; v0.564 adds desktop macOS x64 selection
and classic RW-to-RX publication. Full native macOS acceptance remains pending.

На v0.563 добавлен offline encoder; v0.564 подключает выбор `macos-x64` и
классическую публикацию RW→RX. Нативная приёмка macOS ещё не выполнена.

## Memory and host policy / Память и политика host — v0.564

[`FVM_MAC_CODE_SUPPORTED`](../ports/mac/code_platform.h#L15) requires Apple/Mach, `TARGET_OS_OSX`, x86-64 and the
ordinary 64-bit pointer ABI. It excludes Apple Silicon arm64/arm64e, iOS and other
Apple targets. [`fvm_jit_host_target`](../src/jit/backend.cpp#L8) selects [`FVM_JIT_TARGET_MACOS_X64`](../src/jit/backend.hpp#L19) and
[`fvm_jit_backend_name`](../include/flashvm/jit.h#L19) reports `macos-x64`. This selects the process ISA; a binary
running through Rosetta has not been validated here. Availability describes the
compiled ABI, not entitlement approval for an allocation.

[`FVM_MAC_CODE_SUPPORTED`](../ports/mac/code_platform.h#L15) проверяет Apple/Mach, desktop macOS, x86-64 и обычный
64-битный ABI. arm64/arm64e, iOS и другие Apple-платформы не включаются. Выбор
описывает ISA процесса; Rosetta здесь не проверена. Наличие backend не является
проверкой разрешений подписи приложения.

The [Darwin adapter](../ports/mac/code.c) uses `sysconf(_SC_PAGESIZE)` to round
allocation size without overflow. `mmap` creates independent anonymous private
RW pages using MAP_ANON and descriptor -1. Code is copied, `mprotect` changes the
whole allocation to RX, then `sys_icache_invalidate` prepares the copied range for
execution. Outputs publish after those steps. No RWX mapping, writable alias,
per-thread write-protection switching or shared process arena is used.

Переходник узнаёт размер страницы, проверяет переполнение и выделяет отдельную
анонимную RW-область. После копирования она целиком переводится в RX; затем
синхронизируется кеш и публикуются выходы. RWX, writable alias и переключения
прав текущего потока не применяются.

Internal `fvm_port_code_create(bytes, size, out, mapped)` requires nonnull bytes,
nonzero size and two nonnull outputs. On this supported host, invalid arguments
or page-rounding overflow return INVALID_ARGUMENT; an unusable page query returns
UNSUPPORTED. A reported mmap ENOMEM returns OUT_OF_MEMORY, other allocation or
protection errors return UNSUPPORTED. Errors preserve outputs; a failed protection
change unmaps acquired storage. The `mapped` release cookie is the page-rounded
allocation length, while public machine-code views/sizes exclude padding.
[`fvm_port_code_release`](../ports/code_port.c#L93) unmaps an inactive allocation; null/zero needs no work.
No managed allocation, VM callback or GC occurs in these adapter operations.
The host must exclude active execution, debugger reads and concurrent release.

Неверные аргументы/переполнение дают INVALID_ARGUMENT; ошибка размера страницы,
запрет выделения или защиты — UNSUPPORTED; mmap ENOMEM — OUT_OF_MEMORY. Выходы
при ошибке неизменны; после неудачного mprotect полученная память освобождается.
Внутренний mapped — округлённая длина для unmap, публичный размер — только код.
Освобождение требует отсутствия активных кадров и параллельного использования.
Операции не создают managed-объекты и не вызывают VM/GC.

This is the **classic Intel unsigned-code policy**, not Apple's MAP_JIT policy.
A hardened host must deliberately permit this policy, for example through
`com.apple.security.cs.allow-unsigned-executable-memory`; `allow-jit` alone is
intended for MAP_JIT and does not establish permission for this adapter. Reported
mapping/protection denial permits the existing interpreter fallback. A host must
still configure and test its code-signing/runtime policy before executing JIT;
handling returned errors is not a guarantee against OS termination under an
incompatible execution policy. The library never changes signatures or
entitlements. MAP_JIT, its arena constraints, per-thread/callback write protection,
Apple Silicon and native hardened-runtime acceptance remain separate work.

Это классическая Intel-политика unsigned code, не MAP_JIT. Hardened host сам
выбирает и проверяет разрешения подписи; allow-jit не заменяет политику unsigned
memory этого переходника. Возвращённый запрет публикации допускает интерпретацию,
но не гарантирует защиту от завершения процесса ОС при несовместимой политике
исполнения. Библиотека не меняет подписи или entitlements. MAP_JIT/Apple Silicon
и нативная hardened-runtime приёмка остаются открытыми.

## Build and example / Сборка и пример

On a macOS x64 process with Xcode Command Line Tools and CMake, build the same C89
example. These commands are preparation instructions, not runs performed here:

```sh
cmake -S . -B build-macos -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=x86_64 -DFVM_DEFS_FILE=cmake/profiles/minimal-jit.defs
cmake --build build-macos --target flashvm_examples_embedding_jit_main flashvm_tests_jit_macos_native
./build-macos/flashvm_examples_embedding_jit_main
ctest --test-dir build-macos --output-on-failure -R jit_macos_native
```

If the host deliberately chooses the classic Intel policy with Hardened Runtime,
its signing setup can use the example's
[optional entitlement plist](../examples/embedding/jit/macos-intel.entitlements):

```sh
codesign --force --sign - --options runtime --entitlements examples/embedding/jit/macos-intel.entitlements build-macos/flashvm_examples_embedding_jit_main
```

This is local ad-hoc signing preparation, not a distribution/notarization guide;
apply a real host's chosen identity/policy separately. No signing is performed by
CMake or the library. The example's source/API list and ownership are documented
in [its README](../examples/embedding/jit/README.md). No additional multimedia SDK
is needed for the minimal profile; Apple SDK and system frameworks are required.

Команды готовят сборку и необязательную локальную ad-hoc подпись. Это не инструкция
нотарификации/распространения. Подпись не выполняется автоматически. Требуются
Apple SDK/системные frameworks; дополнительных мультимедиа SDK профиль не требует.

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

`tests/jit_macos_code_port.cpp` runs the actual adapter against controlled SDK
functions: exact RW→RX/cache/unmap order, 4/16 KiB rounding, aligned allocations,
page query failures, overflow, ENOMEM/access denial, failed protection cleanup and
unchanged outputs. `tests/jit_macos_pipeline.cpp` uses the actual adapter with real
Linux mmap/mprotect and a cache-control shim, then runs host/SWF4–8 AVM1 callbacks
through Darwin-framed code, including collecting conversion, exception recovery,
active-code destroy rejection and root/activation cleanup. This remains Linux
pipeline evidence. A separate macro fixture checks the actual Darwin host
selector and pointer-width availability prerequisite.

`tests/jit_macos_native.cpp` is a native macOS Intel acceptance fixture. It checks
compile/attach/invoke/result 42, copied source and detach/destroy, or returns the
CTest skip status when the OS reports mapping denial. On other platforms its
body is explicitly skipped. The existing Windows-only OS unwind fixture likewise
now skips on non-Windows; neither is counted as an executed foreign OS check.

SDK fixtures и Linux pipeline не считаются macOS-приёмкой. Отдельный нативный
тест проверяет результат 42 и время жизни source/code на macOS; чужая ОС или
сообщённый запрет публикации помечаются Skipped.

## Primary references / Первичные источники

- Apple: [64-bit Intel ABI](https://developer.apple.com/documentation/xcode/writing-64-bit-intel-code-for-apple-platforms).
- Apple: [mmap](https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man2/mmap.2.html).
- Apple: [mprotect](https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man2/mprotect.2.html).
- Apple: [instruction-cache synchronization](https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man3/sys_icache_invalidate.3.html).
- Apple: [classic unsigned-memory entitlement](https://developer.apple.com/documentation/bundleresources/entitlements/com.apple.security.cs.allow-unsigned-executable-memory).
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
| `fvm_jit_backend_name` | function | [include/flashvm/jit.h:19](../include/flashvm/jit.h#L19) |
| `fvm_jit_encode` | function | [src/jit/backend.hpp:38](../src/jit/backend.hpp#L38) |
| `fvm_jit_host_target` | function | [src/jit/backend.cpp:8](../src/jit/backend.cpp#L8) |
| `FVM_JIT_TARGET_MACOS_X64` | enum constant | [src/jit/backend.hpp:19](../src/jit/backend.hpp#L19) |
| `FVM_MAC_CODE_SUPPORTED` | macro / define | [ports/mac/code_platform.h:15](../ports/mac/code_platform.h#L15) |
| `fvm_port_code_create` | function | [ports/code_port.c:40](../ports/code_port.c#L40) |
| `fvm_port_code_release` | function | [ports/code_port.c:93](../ports/code_port.c#L93) |
<!-- source-links:end -->
