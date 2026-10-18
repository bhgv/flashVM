# Darwin ARM64 integer-call encoder / Генератор Darwin ARM64

## Scope / Область реализации

v0.568 adds the private offline target [`FVM_JIT_TARGET_MACOS_ARM64`](../src/jit/backend.hpp#L18). It lowers
the existing decoded helper-call plan; it does not directly compile arbitrary
C/C++, Swift, floating-point/variadic calls or arm64e authenticated pointers.
There is no new public C ABI. [`fvm_jit_host_target`](../src/jit/backend.cpp#L6) still excludes Apple Silicon:
this checkpoint does not enable native ARM64 execution on macOS.

Добавлен отдельный offline target: он генерирует машинные инструкции для уже
подготовленного плана вызовов JIT. Наличие байтов не означает разрешения ОС на
их исполнение. Автоматическое включение Apple Silicon, MAP_JIT/write-protection,
подпись/entitlements и нативная приёмка ещё не реализованы этим изменением.

## Calling convention / Соглашение вызовов

Apple's [ARM64 ABI documentation](https://developer.apple.com/documentation/xcode/writing-arm64-code-for-apple-platforms)
reserves X18 and requires a valid frame record through X29. The separate
[`fvm_jit_machine_macos_arm64`](../src/jit/machine.hpp#L32) prologue preserves X19/X20, saves X29/LR at SP+16
inside a 32-byte aligned frame and links X29 to that record before calling a
helper. The epilogue restores the entire frame on normal and error exits.
X18 is never used. The emitter neither consumes nor stores red-zone data.

X19 retains the invocation pointer across helper calls. Each shared
[`fvm_jit_machine_arm64`](../src/jit/arm64.cpp#L17) call sequence reinstalls X0 and a zero-extended unsigned
32-bit operand in W1, loads the full helper address into X16, then executes BLR.
The shared [`fvm_jit_encode`](../src/jit/backend.hpp#L30) relocation path patches AArch64 B instructions,
including backwards branches and conditional branches beyond the CBZ range.
These fixed two-argument helpers do not require stack arguments or variadic rules.
No unwind-table registration or pointer authentication is provided.

X19 хранит полный указатель invocation; X0 и W1 восстанавливаются перед каждым
вызовом. X16 содержит полный адрес функции, X18 не используется. Новый пролог
сохраняет X19/X20 и связанную пару X29/LR; стек остаётся выровненным на 16 байт.
Общий генератор вызовов используется повторно только для совместимой части ABI,
а не для Linux-пролога, в котором нет Darwin frame chain. Ошибка обработчика
переходит в общий эпилог с восстановлением сохранённых регистров и стека.

## Reproduce / Воспроизведение

Build the minimal automatic-JIT profile and emit fresh synthetic-address fixtures:

```sh
cmake -S . -B build-jit -DFVM_DEFS_FILE="$PWD/cmake/profiles/minimal-jit.defs" -DCMAKE_BUILD_TYPE=Release
cmake --build build-jit --parallel 4
mkdir -p build-jit/encoder-fixtures
build-jit/flashvm_tests_jit_backends build-jit/encoder-fixtures
python3 -m pip install unicorn
python3 scripts/check_jit_backends.py build-jit/encoder-fixtures
ctest --test-dir build-jit --output-on-failure
```

The emulator checker is optional, requires Python 3 with assertions enabled and
Unicorn, and executes actual emitted instructions. The CTest generator itself
checks output generation, pointer widths and rejected branch destinations; it is
not equivalent to emulator execution. Keep fixtures/logs in ignored build output.

Проверки Unicorn: прямые/обратные переходы, ранний ошибочный возврат, дальние
переходы через тело более 1 МиБ, старший бит unsigned operand, адреса функций
и invocation выше 4 ГБ, выравнивание стека, сохранение X19/X20/X29 и X18, реальные
связанные frame records и восстановление caller LR. Обработчики намеренно портят
регистры аргументов. Неверная цель перехода оставляет выходной буфер без изменений.
Пять прежних offline-архитектур проверяются тем же запуском.

These are CPU-emulator checks on Linux, not an Apple SDK build, native Apple
Silicon run, OS crash-backtrace test or full AVM1 equivalence evidence. All nine
roadmap stages remain open. Runtime publication needs a separate macOS ARM64
memory/signing policy and native verification before host selection is enabled.

## Sources / Исходники

- Target and width: [backend.hpp](../src/jit/backend.hpp), [backend.cpp](../src/jit/backend.cpp).
- Darwin frame: [macos_arm64.cpp](../src/jit/macos_arm64.cpp), [frame constants](../src/jit/macos_arm64_constants.h).
- Shared calls/relocations: [arm64.cpp](../src/jit/arm64.cpp), [encode.cpp](../src/jit/encode.cpp).
- Fixtures: [jit_backends.cpp](../tests/jit_backends.cpp), [emulator checker](../scripts/check_jit_backends.py).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_jit_encode` | function | [src/jit/backend.hpp:30](../src/jit/backend.hpp#L30) |
| `fvm_jit_host_target` | function | [src/jit/backend.cpp:6](../src/jit/backend.cpp#L6) |
| `fvm_jit_machine_arm64` | function | [src/jit/arm64.cpp:17](../src/jit/arm64.cpp#L17) |
| `fvm_jit_machine_macos_arm64` | function | [src/jit/machine.hpp:32](../src/jit/machine.hpp#L32) |
| `FVM_JIT_TARGET_MACOS_ARM64` | enum constant | [src/jit/backend.hpp:18](../src/jit/backend.hpp#L18) |
<!-- source-links:end -->
