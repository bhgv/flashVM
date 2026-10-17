# Build-selected automatic JIT / JIT по умолчанию при сборке

The `JIT` execution key in DEFS selects `FVM_EXECUTION_JIT` for every newly
created context. Supported bodies compile/cache on entry; unsupported bodies or
hosts without a native backend use the existing interpreter fallback. The key
changes initial policy, not native instruction coverage or platform support.
`INTERPRETER` preserves the previous `FVM_EXECUTION_MANUAL` context default:
explicit native body attachments work, otherwise source is interpreted.

Both profiles include runtime mode switching. `fvm_execution_options_init`
continues to select `FVM_EXECUTION_INTERPRETER`; passing initialized options
therefore explicitly overrides the build default. Null options preserve current
context policy. Changing one context never changes another. Graph transport
moves source, not generated code, caches or the sender's execution policy.
`FVM_Config.features` is unchanged; `FVM_FEATURE_JIT` remains unsupported.

Use `cmake/profiles/minimal-jit.defs` with `-DFVM_DEFS_FILE=/absolute/path` for
a minimal automatic-JIT build. Default `DEFS.txt` still selects INTERPRETER.
The native release runner accepts `--profile minimal-jit` and includes it in
`--profile all`. Both CMake and the standalone Linux reader accept JIT and reject missing or
conflicting execution keys. Native compilation can allocate/cache code and can
return allocation errors; the JIT key does not promise allocation-free execution.

`execution_build_default` checks profile selection, repeated native entry/cache
use where available, unsupported-body fallback, explicit interpretation,
independent contexts and graph transfer. `execution_modes` covers explicit
options. Real foreign CPU/OS runs and all nine roadmap stages remain open.

Ключ `JIT` задаёт новым контекстам автоматическую компиляцию поддерживаемых тел
и fallback в интерпретатор. На машине без native backend исполнение остаётся
интерпретируемым. Ключ `INTERPRETER` сохраняет прежний manual-режим: явно
подключённые native тела работают, остальные исполняются интерпретатором.

API переключения доступен в обеих сборках. Инициализированные через
`fvm_execution_options_init` опции по-прежнему явно выбирают интерпретатор;
нулевой указатель сохраняет текущую политику контекста. Контексты независимы,
перенос графа не переносит режим отправителя и кеш машинного кода. Runtime-бит
`FVM_FEATURE_JIT` остаётся неподдерживаемым. Профиль не расширяет инструкции
backend и не подтверждает запуск на других ОС/CPU. Ошибки выделения памяти
при компиляции остаются наблюдаемыми. Все девять этапов роудмапа открыты.

The full automatic-JIT profile exposed a receiver-discovery difference in
`BitmapData.loadBitmap`: root native bodies retained receiver values but did not
publish receiver frame metadata used by resource lookup. Native activation now
publishes physical and implicit receiver frames in interpreter order, restoring
the suspended chain on success and every failure. `bitmap_load_resource` runs
its resource, GC and graph-transfer scenarios in interpreter, manual and automatic
JIT modes, with the execution receiver deliberately different from global _root.
`jit_references` now explicitly selects its intended manual comparison policy.

Полный профиль выявил выбор неверного ресурса в `BitmapData.loadBitmap`: native
кадр хранил значения receiver, но не публиковал метаданные кадров для поиска
ресурсов. Теперь physical/implicit receiver видны в том же порядке, что у
интерпретатора, а цепочка корней восстанавливается при любом выходе. Ресурсный
тест выполняется в трёх режимах, включая GC и перенос графа с отличающимся _root.

Validation / проверки: Linux x86-64, GNU C/C++ 13.3.0, CMake/CTest 4.4.4,
strict C90/C++98 Release with assertions: 1096 CTest cases passed in each of
minimal and minimal-jit. Thirty build-definition cases and ten platform-evidence
unit tests passed. Debug ASan/UBSan passed bitmap_load_resource,
execution_build_default and script_native_function2 with stack-use-after-return
instrumentation enabled and LeakSanitizer disabled. Initial full runs exposed
the resource regression and the implicit test policy above; generated ELF execute
permissions were also repaired before final full passing runs. These are Linux
host results, not foreign-platform or complete SWF conformance evidence.

На Linux x86-64 прошли по 1096 тестов в двух профилях и три целевые проверки
ASan/UBSan. У сгенерированных ELF восстанавливались потерянные execute-биты;
итоговые полные прогоны успешны. LeakSanitizer отключён. Результаты не заменяют
приёмку на Windows/macOS/Android и всех остальных CPU.
