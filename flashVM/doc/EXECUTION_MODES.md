# Interpreter/JIT modes and diagnostic replay — v0.347

`flashvm/execution_mode.h` selects execution at ordinary entry points, without
manual native-body binding:

```c
FVM_ExecutionOptions options;
fvm_execution_options_init(&options);
options.mode = FVM_EXECUTION_JIT;
options.flags = FVM_EXEC_REPLAY_ON_EXCEPTION; /* optional: repeats effects */
status = fvm_avm1_execute_options(context, actions, size, &options);
/* Callable from a running callback: affects subsequent body entries. */
fvm_execution_set_mode(context, FVM_EXECUTION_INTERPRETER, 0);
```

[`fvm_function_invoke_options`](../include/flashvm/execution_mode.h#L101) provides the same options for a first-class function.
Both wrappers use normal framing, argument/this/super/locals/register setup and
GC roots. Null options retain the context's current policy. Options persist;
they are not silently restored after a call. [`fvm_execution_get_options`](../include/flashvm/execution_mode.h#L84) reports
requested policy. Each context owns its own policy/cache/counters.

- [`FVM_EXECUTION_INTERPRETER`](../include/flashvm/execution_mode.h#L14) executes bytecode and bypasses attached JIT bodies.
- [`FVM_EXECUTION_JIT`](../include/flashvm/execution_mode.h#L15) compiles supported root/function bodies on entry and uses
  a bounded context-owned cache. Unsupported bodies/hosts/cache exhaustion fall
  back to the interpreter. Unsupported bodies are negatively cached. Compilation
  OOM remains an error; it does not enter partially generated code.
- [`FVM_EXECUTION_MANUAL`](../include/flashvm/execution_mode.h#L13) preserves the previous embedding ABI: ordinary functions
  interpret, explicit native-body attachments execute. Fresh contexts use this
  compatibility mode; `options_init` explicitly selects the interpreter.

Native C callbacks/trusted host body bridges remain callable in every mode.
The explicit JIT ABI is identified and bypassed in interpreter mode; arbitrary
host callbacks are not replaced with their placeholder bytecode.
Switching mode does not patch/deoptimize an active native frame. That frame
finishes its selected code, while nested/new entries observe the new policy.
During diagnostic replay, all JIT selections are suppressed until the retry ends;
requested mode changes made during that interval still persist afterwards.
No cross-thread mode changes are supported without external synchronization.

## Replay, logs and counters

Replay is disabled by default. With [`FVM_EXEC_REPLAY_ON_EXCEPTION`](../include/flashvm/execution_mode.h#L18), an emitted
body returning **[FVM_EXCEPTION](../include/flashvm/flashvm.h#L50)** is retried once in the interpreter with fresh
activation setup and the original arguments/source snapshot. This is diagnostic
re-execution, not rollback: globals, objects, I/O and other effects can happen
again. It does not catch CPU faults, recover from arbitrary process crashes,
retry OOM/budget errors or replay an interpreter-only failure.

The retry has a separate bounded instruction budget. Original budget usage and
pending exception/fault information are restored afterwards. The original thrown
value remains a GC root even while pending state is cleared for the retry. The
original native exception is always returned, including when the interpreter
succeeds or raises a different exception. A retry cannot recursively replay.
Preparation OOM leaves the original exception intact; retry OOM is logged as the
retry result without replacing the original throw.

[`fvm_execution_set_logger`](../include/flashvm/execution_mode.h#L88) installs a synchronous host-owned logger; null/null
uses stderr. Events are original exception, retry start, interpreted checkpoint
(original byte offset/counted function label), and retry completion/status.
The logger must not reenter execution, collect, throw or destroy the context.
It can query counters or change requested policy. Ordinary native execution
emits no instruction trace. [`fvm_execution_get_stats`](../include/flashvm/execution_mode.h#L92) reports native/interpreter
entries, successful automatic compilations, cache hits/fallbacks, retries,
retry checkpoints and the last retry status. Counters are context-local unsigned
values, not timing/performance measurements or proof of compiler coverage.

## Run the example

```
cmake -S . -B build
cmake --build build --target flashvm_examples_embedding_execution_modes_main
./build/flashvm_examples_embedding_execution_modes_main
```

On the tested Linux x86-64 backend it prints interpreter and JIT results 42.
It then intentionally throws 111 in native execution, emits interpreter diagnostic
steps to stderr, and reports one successful retry while preserving exception 111.
On unsupported hosts the numerical path remains interpreted and the native
exception demonstration is skipped. Both INTERPRETER and JIT build profiles support these runtime options.
JIT seeds new contexts with automatic compilation/fallback; INTERPRETER retains
the manual default. Explicit options initialization still selects interpreter.
The runtime configuration feature bit remains reserved.

The native compiler is still the subset in [JIT.md](JIT.md). Other instructions
continue to work only to the extent supported by the interpreter; selecting JIT
mode does not make unsupported Flash behavior implemented. Full JIT, all requested
CPU backends and roadmap stages 8/9 remain open.

Evidence: `execution_modes` checks C89 options, root/function entry, positive and
negative caching, unsupported fallback and interpreter selection.
`execution_mode_switch` changes policy inside a native helper and exercises both
nested paths while the outer native frame stays valid. `execution_mode_replay`
checks fresh retry, GC of the preserved thrown graph, same/different retry status,
step events and exact original exception restoration. `execution_mode_root_replay`
checks head-function replay, default stderr logging and restored original budget.
`execution_mode_legacy_scope` checks SWF5 local-this casing for named records
without a first-class callee. Allocation cases cover cache compilation/insertion,
invocation, retry preparation and retry OOM. Existing native tests and the example
remain executable regression coverage. Memory profiles retain their documented
heap/global ASan+UBSan restrictions; no stack-ASan/LeakSanitizer or other-CPU/OS
execution claim is made.

## Русский

Добавлены опции головного вызова [`fvm_avm1_execute_options`](../include/flashvm/execution_mode.h#L96) и вызова функции
[`fvm_function_invoke_options`](../include/flashvm/execution_mode.h#L101). Режим можно изменить через
[`fvm_execution_set_mode`](../include/flashvm/execution_mode.h#L79) прямо из выполняемого callback: новые/вложенные вызовы
увидят переключение, активный машинный кадр завершится в прежнем режиме.
В режиме JIT поддерживаемые тела компилируются и кешируются автоматически;
остальные исполняются интерпретатором. Кеш принадлежит контексту и освобождается
при его уничтожении. Настройки сохраняются между вызовами.

Повтор после исключения включается только флагом [`FVM_EXEC_REPLAY_ON_EXCEPTION`](../include/flashvm/execution_mode.h#L18).
Функция/корневой блок выполняется заново в интерпретаторе с логом шагов и
статистикой. Создаётся свежая активация, но состояние приложения не откатывается:
побочные эффекты могут повториться. Возвращается исходное исключение JIT;
результат диагностического прохода хранится в статистике и логе. Даже при GC,
другом исключении или OOM в повторе исходная ошибка сохраняется. Рекурсивные
повторы запрещены. Диагностический проход имеет отдельный ограниченный бюджет.

Для обычного JIT нет подробного лога инструкций. Для диагностики предусмотрен
callback логгера либо stderr по умолчанию, а также счётчики native/interpreter,
кеша, fallback и шагов повтора. Пример запуска приведён выше; он показывает оба
режима и реальный сценарий native exception → interpreter replay → log/stats.
Это работающая интеграция первого backend, а не завершение полного JIT.

For architecture builds, breakpoint-driven interpretation, independent OS workers
and multiple simultaneous VM instances, see [JIT_ARCHITECTURES.md](JIT_ARCHITECTURES.md).

Stored frame blocks also honor automatic selection as of v0.368, including Call,
frame actions and initialization blocks. Their active named breakpoint exclusion
precedes native selection; unsupported bodies retain interpreter fallback. See
[JIT_DISPATCH.md](JIT_DISPATCH.md) for the tested frame-entry scope and limits.

Bounded frame replay (v0.369): stored frame diagnostic retries use the same
implicit-End acceptance as their original entry; ordinary roots keep their normal
framing checks. / Диагностический повтор сохранённого кадра сохраняет допустимый
неявный End; обычные корневые входы сохраняют прежнюю проверку границ.
See `execution_mode_frame_replay` and [dispatch contracts](JIT_DISPATCH.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_avm1_execute_options` | function | [include/flashvm/execution_mode.h:96](../include/flashvm/execution_mode.h#L96) |
| `FVM_EXCEPTION` | enum constant | [include/flashvm/flashvm.h:50](../include/flashvm/flashvm.h#L50) |
| `FVM_EXEC_REPLAY_ON_EXCEPTION` | macro / define | [include/flashvm/execution_mode.h:18](../include/flashvm/execution_mode.h#L18) |
| `fvm_execution_get_options` | function | [include/flashvm/execution_mode.h:84](../include/flashvm/execution_mode.h#L84) |
| `fvm_execution_get_stats` | function | [include/flashvm/execution_mode.h:92](../include/flashvm/execution_mode.h#L92) |
| `FVM_EXECUTION_INTERPRETER` | enum constant | [include/flashvm/execution_mode.h:14](../include/flashvm/execution_mode.h#L14) |
| `FVM_EXECUTION_JIT` | enum constant | [include/flashvm/execution_mode.h:15](../include/flashvm/execution_mode.h#L15) |
| `FVM_EXECUTION_MANUAL` | enum constant | [include/flashvm/execution_mode.h:13](../include/flashvm/execution_mode.h#L13) |
| `fvm_execution_options_init` | function | [include/flashvm/execution_mode.h:74](../include/flashvm/execution_mode.h#L74) |
| `fvm_execution_set_logger` | function | [include/flashvm/execution_mode.h:88](../include/flashvm/execution_mode.h#L88) |
| `fvm_execution_set_mode` | function | [include/flashvm/execution_mode.h:79](../include/flashvm/execution_mode.h#L79) |
| `FVM_ExecutionOptions` | struct | [include/flashvm/execution_mode.h:23](../include/flashvm/execution_mode.h#L23) |
| `fvm_function_invoke_options` | function | [include/flashvm/execution_mode.h:101](../include/flashvm/execution_mode.h#L101) |
<!-- source-links:end -->
