# Compiler / IDE continuation acceptance, 2026-10-24

[Workspace](../../README.md) · [Limbo scopes](../../fir/v1/doc/LEXICAL_SCOPES.md) ·
[Navigation example](examples/navigation/README.md) · [Protocol](PROTOCOL.md)

This series delivers FIR tools 0.5.3 and IDE 0.3.1 on the published 0.2.8 IDE
checkpoint. FlashVM's own version and runtime sources are unchanged. Source and
IDE are independent components with explicit external build/test tool paths.
All work is exported as local commits for git am; nothing is pushed by the agent.

## Implemented and checked

- The authoritative Coco/R numeric-source grammar accepts bounded Limbo lexical
  blocks with shadowing and persistent writes to outer bindings. The new test
  covers eight valid source/verification/optimization cases, nine failed inputs
  preserving the existing destination and 16 real SWF trace comparisons (original
  and optimized). Input recursion is reset after failures in the embedding API.
- FIR native CTest: 9/9. The full ASan/UBSan FIR suite also passed 9/9; final focused
  source acceptance, embedding and scope checks passed after the last equivalent
  integer-return/layout cleanup. VM and SWL in these runs are ordinary native
  builds, not instrumented dependencies. LSan and ASan fake-stack use-after-return
  checks are disabled with a host-only linked defaults shim because /proc is
  unavailable; the first environment-only LSan attempt failed during teardown.
- IDE reference CTest: 14/14, including the independent C99 symbol-prefix reader.
  Six modules in each reference mode check 765 assertions: original state 145,
  limits 21, reduced revisions 16, coordinates 430, Find 137 and example 16.
  The additional host admission test covers 46 process-double scenarios.
  Both process exit and a PASS marker are required for each Dis run.
- The emitted module is 2975 bytes and its .sbl is 5796 bytes with this explicit
  compiler. The independent Coco/R/C99 reader accepts two source files, 465
  logical PCs and a 4648-byte source-map prefix. This is not complete .sbl/type/
  local validation or binding debug data to a loaded module instance.
- Detached renamed FIR and IDE copies, each accompanied by LICENSE, build using
  explicit tools. Detached FIR passes 9/9 with real runtime paths; detached Dis
  passes 14/14 and detached host Python tests pass 25/25. Installation contains
  only idecore.dis, idecore.sbl and idecore.m; examples/tests are not installed.
- Original host Python tests pass 25/25. ATG canonical layout, changed-document
  local file targets and git whitespace checks pass. Reference string lifetime
  caveat and fixture construction are documented [separately](REFERENCE_RUNTIME.md).

## Reproducible tool identity and limits

Linux x86-64, C99 native tools, CMake 4.4.4, Python 3.12.14.
Coco/R C generator: `312d377d2bdf7b2e180b0801a89ca651950a4873`.
External bootstrap compiler/headless emulator: InferNode
`abb1b6860238f89eb5a10125e561eff4e0a34836`. Bootstrap needs yacc; the host build used
that repository's own iyacc. These tools are not vendored or runtime dependencies
of the deployed IDE. Final acceptance uses the unmodified upstream emulator
source; temporary native backtrace instrumentation was removed.

Reference -c1 is not instrumented native JIT evidence. Execution inside the user's
Inferno experiment, embedded FlashVM Dis, x86-64/arm64/riscv64 JIT parity, full AS/
Limbo semantic compilers, ABI bridging, filesystem/editor UI and execution debug
control remain open. The numeric AS3 output remains AVM1 compatibility, not ABC.
Reply changed at 0.3.0: rebuild all clients with the installed idecore.m.

## Русский

Серия даёт FIR 0.5.3 и IDE 0.3.1: блоки/затенение Limbo, координаты документа и
точный линейный поиск через типизированные каналы с проверкой ревизии. FIR проходит
9/9 CTest, включая реальные SWF при явно выбранных SWL/runner; новый scope-тест
даёт 16 исполнений исходного/оптимизированного ролика и девять rollback-ошибок.
ASan/UBSan проходит полную серию 9/9, затем три focused проверки после последней
эквивалентной правки оформления/return; VM/SWL обычные native builds. LSan и ASan
fake-stack use-after-return отключены host-only shim из-за отсутствующего /proc;
первая попытка отключить LSan только environment-параметром не сработала.

Dis проходит 14/14 CTest. На каждый reference-режим приходится 765 утверждений;
host guard проверяет 46 сценариев. Отделённые переименованные копии проходят те же
9 FIR / 14 Dis tests, а Python host tests — 25. Продукт: 2975-byte .dis и 5796-byte
.sbl; независимый Coco/R reader принимает 465 PC mappings. Это не полный verifier
и не execution debugger. Известный сбой конкатенации внешней VM описан отдельно.
Пользовательская experiment и native JIT здесь не исполнялись. Reply ABI изменён:
ядро и всех клиентов нужно пересобрать с одним idecore.m. Push не выполняется.
