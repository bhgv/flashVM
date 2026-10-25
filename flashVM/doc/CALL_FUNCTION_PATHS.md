# Scoped function calls / Вызовы функций по путям — v0.263

[`fvm_exec_call_function`](../include/flashvm/execution.h#L577) resolves an already-coerced counted name using explicit
lexical scope, target, this and default receiver inputs. It accepts ordered value
arguments, without bytecode or interpreter-stack access. The interpreter retains
name coercion before argument-count coercion.

Loaded SWF4 uses colon qualification; SWF5–8 also recognize dotted and slash paths.
Qualified members use the resolved object as their receiver and retain the
prototype defining the member for superclass dispatch. Plain names retain the
existing With/default receiver selection. Missing or non-callable loaded bindings
return undefined, without resurrecting a deleted entry from the script registry.
Unloaded host calls retain their existing literal-name and registry fallback.

[GC:] Arguments, environment, callee, receiver and defining owner are retained
through getter callbacks and invocation. Invocation metadata is restored on
allocation failure and exceptions. Source-owner identity is captured before a
getter can delete the defining property. This preserves the existing live
prototype-depth superclass model; arbitrary prototype detachment does not freeze
the old superclass chain.

Fixtures: `execution_call_function`, `avm1_call_function_paths`,
`execution_call_function_callbacks`, `execution_call_function_allocation` and
`avm1_call_function_super_owner`. These cover C89 callers, counted names, SWF4–8,
deleted cached definitions, inherited super calls, getter/callee GC, exception
output preservation and allocation failure. Full virtual super-path lookup,
metatable behavior and complete scope equivalence remain open.

Общий C-интерфейс получает готовое строковое имя и значения аргументов, без
байткода и стека интерпретатора. Составное имя сохраняет настоящий получатель и
прототип определения метода для `super`; отсутствующее или невызываемое значение
в загруженном SWF возвращает undefined. Удалённая функция не восстанавливается
из старого кэша. Корни GC и состояние вызова сохраняются при getter, исключениях
и отказах памяти. Полная совместимость областей видимости и JIT ещё не завершена.

Reference: pinned Ruffle
[`activation.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs),
`action_call_function` and `get_variable`.

Validation: Linux GCC13.3 AGG and Cairo each pass 502 tests (Cairo required
rerunning three binaries after restoring lost execute bits). All 26 unchanged
original SWFs match exact traces in both profiles. The selected 17-test
ASan/UBSan run passes; this environment covers heap/global checks, without
stack or leak instrumentation. No non-Linux or generated-machine-code result
is claimed.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_call_function` | function | [include/flashvm/execution.h:577](../include/flashvm/execution.h#L577) |
<!-- source-links:end -->
