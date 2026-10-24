# Compiler diagnostics / IDE history acceptance, 0.4.0

Date: 2026-10-24. [Русский — ниже](#русский).
[History](HISTORY.md) · [Diagnostic contract](../../fir/v1/doc/SOURCE_DIAGNOSTICS.md) ·
[First steps](../../README-FIRSTSTEPS.md) · [Reference runtime caveat](REFERENCE_RUNTIME.md).

Base: published FlashVM `fcd5e7a12b08b36c2b19def5443f9d2d238ba9b4`.
This series advances FIR to 0.5.4, IDE to 0.4.0 and the workspace documentation
to 0.1526. FIR text remains 1.0, SWO remains 1, and Request/Reply layouts are
unchanged. Undo/Redo opcodes append after Find.

## Implemented

- Additive `fir_source_ex` returns a copied first-failure struct. Original UTF-8
  byte spans, physical LF/byte columns and Coco/R syntax codes remain separate
  from legacy scanner prose. Target names/operators are captured before RHS
  parsing. All borrowed diagnostic/source/stream pointers retire on return.
- CLI opts into one schema-1 stderr JSON record on failure. Success stays silent.
  Unknown/duplicate options fail explicitly. Rejected translation retains the
  destination; final stream-copy failure is not promised filesystem rollback.
- Host IDE opts into this schema, rejects ambiguous/malformed records and verifies
  positions against its exact captured bytes. It converts complete UTF-8 boundaries
  to character offsets, retains byte fields and identifies the snapshot by SHA-256.
  A live edit cannot relabel that snapshot; the editor must still check digest/revision.
  Deeply nested JSON and surrogate messages remain raw unlocated logs.
- Limbo channel owner implements Undo/Redo with new revision identities, saved
  state unchanged, Redo retained on no-op/rejected edits and save acknowledgement,
  branch retirement, closed-session isolation and bounded persistent history.
  Combined budgets are 64 snapshots and 1048576 logical characters. These do not
  bound measured heap bytes or catch allocation/runtime traps.
- Fully commented C99 and Limbo tutorials, bilingual contracts and cross-linked
  entry points. Root Bash/Python menus run navigation or history explicitly;
  IDE test selection also runs the host tests. Selecting symbols with IDE tests
  supplies the freshly built independent SBL reader through an explicit SDK path.

## Reproducible tools and evidence

Linux AMD64 native tools:

- External bhgv/coco-r-C `312d377d2bdf7b2e180b0801a89ca651950a4873`.
- External headless InferNode `abb1b6860238f89eb5a10125e561eff4e0a34836` supplies
  native Limbo, reference emu and module headers. Bootstrap used its own iyacc
  as yacc. No runtime source/library was imported into FlashVM.
- Explicit CMake/CTest, native C/C++ and Python 3. The host lacks /proc information
  used by some native tools; explicit writable temporary directories were supplied.

| Check | Result | Scope |
| --- | --- | --- |
| Four independent syntax frontends | 35/35 CTest | Recognition, embedding and generator guards |
| FIR Release | 12/12 CTest | C API, CLI JSON, examples, verifier, optimizer and syntax boundary |
| FIR with real FlashVM public-API host + SWL | 12/12 CTest | Original/optimized SWF execution, including channel extension fixtures |
| FIR with UBSan, halt_on_error=1 | 12/12 CTest | Compiler/source adapter instrumented; VM/linker native, not instrumented |
| SWL | 3/3 CTest | Formats, embedding and inspection |
| SBL | 3/3 CTest | Prefix reader, embedding and generator guard |
| IDE Dis with optional independent SBL reader | 20/20 CTest | Actual compiled Limbo in reference modes 0/1, product guard and symbols |
| Host IDE | 30 unit tests | Five structured-diagnostic groups plus prior orchestration cases |
| Actual host CLI pipeline | 16 legacy / 20 structured scenarios | All four numeric profiles, optimized/unoptimized trace and rollback |
| Renamed detached compiler/editor directories containing spaces | FIR 12/12; IDE 20/20 | Explicit external SDK paths, no sibling source dependency |
| Product installation | Exactly dis/m/sbl | No bootstrap or reduced-limit fixtures installed |

The history regression executes 1088 assertions per reference mode; the reduced
3-entry/6-character fixture executes 40 per mode. Terminal revision fixture uses
MaxRevision=4 and executes 21 assertions per mode. History example executes 12
checks per mode, printing first/revision 3/saved 1 and third/revision 6/saved 1.

Production module at this checkpoint: 4080-byte idecore.dis and 8408-byte idecore.sbl.
The independent reader accepts two source files, 644 logical PCs and a 6581-byte
prefix. This is source-map-prefix evidence only, not full type/local symbol
validation, breakpoint control or matching native JIT mappings.

The first-error JSON fixture and malformed/deep/surrogate record cases were
extended after initial acceptance; affected native/UBSan CLI and host tests were
rerun. No relaxed PASS conditions or assertion removal was used.

## Broader VM regression is not green

The unchanged flashVM/ subtree was also built with regression tests and the root
script attempted all 2010 CTests. Three platform tests were skipped. The initial
run had 21 BAD_COMMAND permission failures: those generated test files lacked
execute bits in this workspace. Restoring execute bits on those known generated
binaries and rerunning the 25 failed cases made all 21 pass. This was a local
artifact repair, not a source change or a claim about its underlying cause.

Four failures remain in the unchanged VM subtree:

| Test | Observed failure |
| --- | --- |
| flashvm_tests_jit_frame_dispatch | Invocation-result assertion in frame_dispatch_run |
| flashvm_tests_standard_rectangle | fvm_object_call expected FVM_OK |
| flashvm_tests_swf_movieclip_timeline | Invalid-version load expected FVM_UNSUPPORTED and no clip |
| flashvm_multimedia_tutorial_contract | Existing multimedia source lines reported unexplained |

The root script correctly stopped at this failed VM stage. The compiler/IDE
component sequence was separately built and tested through the same script.
No all-VM-green or new target JIT acceptance is claimed by this patch.

Earlier host timeout-cleanup runs intermittently observed an empty staging
directory at the final cleanup assertion. An isolated baseline timeout test and
subsequent full host runs passed; adding diagnostic inspection changed the
reproduction. No cause, cleanup workaround or allocation-fault guarantee is
claimed. Tests retain the original immediate cleanup assertion, without sleeps,
retries or weakened checks. The reference string-lifetime caveat remains separately
recorded in REFERENCE_RUNTIME.md.

## Re-run the new component sequence

Set native COCOR_ROOT/COCOR_EXECUTABLE, LIMBO_EXECUTABLE, EMU_EXECUTABLE and
LIMBO_MODULE_DIR as in the first-steps guide:

```sh
python3 build-menu.py --projects compilers,fir,linker,symbols,ide --tests --demo history
```

For runtime-enhanced FIR acceptance supply a compatible public-API host built from
[run_flashvm.c](../../fir/v1/tests/run_flashvm.c):

```sh
FIR_SWL=/absolute/build/swl FIR_FLASHVM_RUNNER=/absolute/build/fir-run \
  ctest --test-dir build/workspace/fir --output-on-failure
```

Final patch application is checked from the published base using git am --3way
and comparison of resulting trees. No push is performed. Full language compilers,
Dis execution/embedding from the user's experiment, graphical IDE, persistence,
complete debugger and instrumented x86-64/arm64/riscv64 target JIT acceptance remain
open. Reference -c0/-c1 modes alone are not native JIT proof. No physical Android,
Windows or macOS runtime acceptance was performed in this series.

## Русский

Серия добавляет FIR 0.5.4 с первой структурированной ошибкой и исходными UTF-8
диапазонами, host-проверку координат по захваченному снимку и Undo/Redo в
Limbo-ядре IDE 0.4.0. Грамматики остаются Coco/R, runtime IDE — Dis, Python —
host-инструмент. Новая история восстанавливает текст с новым номером ревизии,
не меняет saved, сохраняет Redo при no-op/отказе и ограничивает обе стороны
64 снимками/1048576 символами. Исходники и примеры подробно комментированы;
контракты переведены и связаны с README-FIRSTSTEPS и меню сборки.

Пройдены: фронтенды 35/35, FIR 12/12, SWL и SBL по 3/3, Dis 20/20, host 30
тестов, настоящая host-цепочка 16 сценариев в legacy и 20 в structured режиме.
FIR прошёл 12/12 с UBSan и реальным public-API host FlashVM; VM/линкер в этом
прогоне не инструментированы. Отдельные переименованные каталоги с пробелами
собрались и прошли FIR 12/12 и Dis 20/20. Установлены только dis/m/sbl.
SBL-проверка принимает 644 логических PC, не доказывая полный отладчик.

Общий прогон неизменённой VM не зелёный: среди 2010 тестов 3 платформенных
пропущены, 21 локальный отказ запуска устранён восстановлением executable bits
только у сгенерированных тестовых файлов. После повторного запуска остаются
четыре отказа из таблицы выше. Корневой скрипт корректно остановился; проверки
новых compiler/IDE-компонентов выполнены отдельно тем же механизмом.

Ранние host timeout-проверки иногда видели пустой staging-каталог после очистки;
baseline и последующие полные запуски проходили, а диагностическое наблюдение
меняло воспроизведение. Причина не установлена, исправление не заявлено.
Исходные утверждения об очистке сохранены без sleep/retry/ослабления.
Ограничение внешней VM по lifetime строк также остаётся открытым.

Полные языки, целевой experiment Dis внутри FlashVM, GUI, persistence, полный
отладчик и новые нативные JIT этой серией не принимаются. Патч применяется
локально через git am --3way; публикация остаётся за пользователем.
