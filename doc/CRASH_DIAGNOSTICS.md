# VM fault diagnostics / диагностика сбоев VM

The C89 API is in `flashvm/crash.h`. Crash action and preservation detail are
independent context policies, configured outside execution:

| Action | Behavior |
| --- | --- |
| FVM_CRASH_NONE | Ordinary failure behavior; no report construction |
| FVM_CRASH_BACKTRACE | Retained JSON with function names, arguments, source/action locations and interpreter/JIT identity |
| FVM_CRASH_ZIP | ZIP with trace, bytecode, raw global/object/class state and available activation locals |
| FVM_CRASH_ZIP_CODE | The same plus bytes of the active native mappings |
| FVM_CRASH_DEBUGGER | Synchronous host debugger callback before activation unwinding |

`fvm_crash_set_detail` independently selects NATURAL (default) or FORCE.
NATURAL borrows existing rooted arguments/activations without copying values at
entry or adding per-instruction local-variable spills. FORCE makes rooted entry
copies of supplied arguments, copies their string bytes and exposes Function2
register parameters under their formal names. Force preservation failure is
reported separately; `frame.forced` states whether the entry copy succeeded.
Objects retain their identity; FORCE does not deep-copy an object's entry-time
contents. Dumped object fields describe fault-time state.

Current native backends already materialize locals and registers through shared
runtime helpers and perform no optimizing elimination. FORCE therefore keeps
real native execution and makes its existing activation/register state explicit;
it does not switch execution to the interpreter. Anonymous registers are shown
by index; names are not invented. Available locals are copied only at failure.
There is no per-instruction local history in either mode.

Frames form an allocation-free borrowed native-stack chain in NATURAL mode.
JIT code already owns its original AVM1 source; fvm_jit_source and
fvm_jit_machine_code expose immutable source and native mapping views. Retained
reports own source copies, so they remain readable after function/code deletion
and collection. ZIP_CODE records the currently failing structured child mapping,
not only the outer function mapping. Original RX load addresses are opaque
postmortem identities. Offsets are AVM1 actions, not native fault PCs.

Failures are **first chance** VM return statuses, including script throws, before
an enclosing AVM1 Try may handle them. Deeper reports are retained through error
propagation; interpreter diagnostic replay does not replace the native report.
Returning from the debugger callback permits normal error unwinding. It does not
retry a faulting instruction or repair VM state. Hosts can block in the callback
for an interactive debugger and use live argument/local/register queries plus
existing table/object/function inspection APIs. No UI is added. Do not execute
script, collect, mutate or destroy the context from this callback.

Reports use bounded copied storage (256 frames; 16 MiB per payload/code-copy
budget). Allocation, section-budget or file-write errors set diagnostic_status
without replacing the original VM error. The debugger callback still runs if
report construction fails. A failed capture may leave an older output file in
place; check diagnostic_status and the current in-memory report before consuming
that path. A null path keeps a ZIP in memory. Configured file paths are explicitly
overwritten on successful capture; output is not uploaded automatically.

ZIP entries use stored ZIP32 records with CRC32. `backtrace.json` preserves raw
value tags; numeric values are strings (including NaN/infinity), strings and
names use reversible JSON byte escapes. Object IDs are snapshot-local pointer
identities, never handles for restoration. `heap.json` contains raw own named and
indexed values, prototypes, metatables, attributes, native-method/accessor names
and available builtin cached values. It includes classes as ordinary objects.
Getters, coercions and user serializers are never invoked. Native resource memory,
remote process state and an executable VM restoration image are not captured.

This implementation handles VM failures on the owning VM thread. It installs no
OS signal/SEH handler and cannot safely inspect a corrupted process from SIGSEGV,
stack overflow or an asynchronous signal. Host/platform crash handlers and native
PC unwinding remain separate work; the APIs must not be called from a signal
handler. The API does not terminate the host process in any mode.

## Русский

Действие при ошибке и полнота сохранения задаются отдельно. Доступны пять режимов:
ничего; бэктрейс; ZIP со значениями и исходным байткодом; ZIP также с активным
машинным кодом; остановка в синхронном callback внутреннего debugger API.

Обычный режим использует уже существующие аргументы, activation и регистры;
локальные значения не копируются на каждой инструкции. FORCE дополнительно
сохраняет входные значения аргументов и строки, удерживает объекты корнями GC и
показывает register-параметры Function2 по именам. Текущий JIT и так хранит
локальные значения в runtime-структурах и не удаляет их оптимизацией: переключать
его на интерпретатор не требуется. Имена неизвестных регистров не выдумываются.
Для объектов сохраняются идентичность аргумента и состояние полей на момент
сбоя, а не глубокая копия всего графа на входе.

Callback выполняется до раскрутки кадров: доступны живые аргументы, локальные
переменные, регистры и существующие API просмотра объектов/функций. Возврат из
callback продолжает обычную обработку ошибки; повтор аварийной инструкции и
исправление повреждённой VM не подразумеваются. Ошибка перехватывается first
chance, в том числе если затем её обработает Try. Диагностический replay не
должен заменять исходный JIT-бэктрейс. UI пока отсутствует.

ZIP содержит CRC, JSON-бэктрейс, AVM1-байткод, сырой снимок объектов/классов и
при выборе ZIP_CODE — активные машинные тела. Getter и пользовательский код не
вызываются. Ошибки памяти/записи видны отдельно от первоначальной ошибки VM.
Режимы не устанавливают обработчик SIGSEGV/SEH: безопасный native crash handler
и раскрутка по native PC требуют отдельной платформенной реализации.

Validation: strict C90/C++98 Linux x86-64 Release suites passed 1101 CTest
cases in both minimal and automatic-JIT profiles. The archive reader checks ZIP
CRC and JSON independently. The native child-mapping test destroys the mapping
and collects before rereading retained data. Allocation-failure retries preserve
original status and invoke the debugger even when report construction fails;
NATURAL frame entry is tested with all new allocations disabled. Three targeted
ASan/UBSan cases passed (stack-use-after-return enabled; LeakSanitizer disabled):
crash_diagnostics, crash_allocation and execution_mode_frame_replay.
Generated ELF execute bits were repaired before final complete passing suites.

Проверки: по 1101 CTest в двух Linux-профилях; независимое чтение ZIP/CRC/JSON,
живые параметры и Function2-регистры, активный дочерний JIT-блок, сохранность
данных после освобождения кода/GC, OOM/retry и исходный JIT-отчёт после replay.
Три целевых ASan/UBSan-теста прошли с указанными выше ограничениями.
