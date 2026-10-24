# Portable Dis core acceptance, 0.2.7

Checked on 2026-10-24, Linux AMD64. External bootstrap compiler/emulator:
InferNode `abb1b6860238f89eb5a10125e561eff4e0a34836` (built headless from source).
The selected bhgv Dis experiment branch was rechecked at
`1d8a3e3d3c9694385786b1df784bbd77c7ef9a75`; it is not the reference test emulator
used here, and no execution inside that branch or FlashVM is claimed.

- Actual product outputs: idecore.dis 1532 bytes, idecore.sbl 2928 bytes at this
  compiler checkpoint. They are build products, not generated source committed
  into the repository. The downloadable module archive contains production files.
- Seven default configured tests pass with the explicit emulator; an eighth
  optional test passes with the selected independent C99 .sbl prefix reader.
- Each mode `-c0` / `-c1`: 145 session/edit/status checks, 21 character-limit/value/
  backpressure/concurrent-writer checks and 13 reduced-limit revision checks.
  Total 179 checks per mode. Both process exit and PASS output are required.
- The revision fixture compiles unchanged production code with a test-only cap of
  4, verifying rejection/no-op/save/shutdown behavior at the cap. This exercises
  the guard algorithm, not billions of requests or a measured native integer trap.
- Six host build-product cases verify nonzero exit, absent/stale outputs, only one
  output, empty module, wrong symbol module name and the successful admission path.
  These fixture bytes are not represented as valid Dis bytecode.
- Detached renamed source plus LICENSE independently builds and passes all seven
  default configured tests with explicit external tools. Installation contains
  only idecore.dis, idecore.sbl and idecore.m under share/flashvm/ide/dis.
- Independent existing C99 .sbl reader accepts module Idecore, two source files,
  230 logical Dis PCs and prefix extent 2214 bytes. Locals/types/debug control and
  binding symbols to a loaded module instance remain separate acceptance work.

No production graphics/system/crypto module dependency was added: idecore.b
includes only idecore.m. Only the test bootstrap uses Sys for reporting and for
halting its own isolated emulator. No Draw context, shell bootstrap or graphical
Inferno subsystem is required by these tests. The external reference emulator
itself is not minimal embedded Dis; its libraries are not imported into FlashVM.

The `-c1` run is not proof of instrumented native-code entry, specific JIT opcode
coverage or real x86-64/arm64/riscv64 target acceptance. Full editor UI, rendering
channels, files/save service, builds, debugger, AS module bridge, embedded Dis
and SWF/Dis combined packaging are not implemented by this checkpoint.

## 0.2.8 host product-signature checkpoint / Проверка сигнатур

2026-10-24, Linux x86-64, Python 3.12.14, CMake 4.4.4:
46 process-double admission contracts pass directly, through the registered
CTest (`ide_dis_compiler_products`, 1/1), and from a detached renamed IDE copy
with LICENSE. The expanded fixture fails against the parent script at
`garbage-module`, demonstrating the original false admission. All 25 existing
host build-backend unit tests pass. `git diff --check` passes.

CTest configuration uses `/usr/bin/true` solely to satisfy the existing explicit
compiler-path requirement; only the host product test runs. No production module
is built with it. No new Limbo compilation, real Dis execution, full source-to-SWF
integration, debugger or JIT/device acceptance is claimed. Earlier real-module
measurements above remain historical evidence, not fresh results for this patch.
The regression fixtures intentionally test signatures only, including successful
magic-only files; they are not valid executable module fixtures.

46 host-контрактов проходят напрямую, через CTest и в отделённой переименованной
копии IDE с LICENSE; прежние 25 unit-тестов host backend также проходят.
На исходном скрипте новый тест падает в `garbage-module`, подтверждая дефект.
При настройке CTest `/usr/bin/true` служит только существующей проверке явного
пути компилятора: продуктовый модуль им не собирается. Новые Limbo compilation,
Dis execution, SWF pipeline, debugger/JIT/device результаты не заявляются.
[Контракт и пример](PRODUCT_ADMISSION.ru.md) / [Contract and example](PRODUCT_ADMISSION.md).

[0.3.1 compiler/IDE acceptance / Приёмка серии](SERIES_0_3_1.md).
