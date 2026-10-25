# Current-global primitive boxing — v0.474

Loaded Object(value), Function.call and Function.apply now use the existing AVM1
primitive-to-object service to select the current global Number, Boolean or String
constructor. They no longer silently construct through the embedding registry or
the adapter's captured original classes after the corresponding global changes.

| Current global binding | Object(primitive), call or construction | Function.call/apply receiver |
| --- | --- | --- |
| Builtin or replacement script constructor | Its constructed Object | Its constructed Object |
| Missing, undefined, null, scalar or non-callable Object | Fresh bare Object | Global Object |
| Getter returning a constructor | Construct the returned class once | Construct the returned class once |
| Getter/constructor error or exception | Propagate without an Object result | Propagate before calling the target |
| Inherited constructor | Select the inherited global binding | Select the inherited global binding |
| Missing binding with __resolve | Skip resolver and use the bare fallback | Skip resolver and use the global fallback |
| Alternate spelling | SWF4–6 updates existing spelling; SWF7–8 remains exact | Same versioned global selection |

The resolver exclusion is deliberate: pinned Value::coerce_to_object uses
get_opt(class_name, activation, false). Captured original constructors remain in
the existing transport graph for unloaded host compatibility. Loaded destinations
ignore those captures for receiver boxing, including when no standard library is
installed there. Unloaded Object conversion and call/apply continue using their
historical registry/capture behavior. No public ABI or wire callback IDs change.

## Tests and remaining limits

- object_global_boxing: host and SWF4–8, three primitive types and fourteen binding
  configurations; direct/named C calls and construction, NewObject/NewMethod and
  CallFunction/CallMethod in interpreter, attached native and automatic JIT modes.
- function_global_boxing: matching call/apply matrix, ordinary and saved adapters,
  prepared arguments, exact fallback receivers and getter/constructor exceptions.
  Script getters really collect on these trusted adapter paths. Object native
  factory paths retain their collector barrier and explicitly reject collection.
- function_global_boxing_lifetime: apply element getters clear the input/constructor
  globals, collect the otherwise private newly boxed receiver and check its counted
  NUL payload; plain error/exception paths suppress target invocation. Transferred
  host adapters run in loaded SWF6/8 contexts without standard installation and use
  those destinations' missing/undefined/custom constructor bindings. Real script
  factory and target bodies participate in interpreted/attached/automatic execution.
- primitive_global_boxing_allocation: new/new[] sweeps through first success for
  SWF6/8, three primitive types, missing/custom bindings and Object/call/apply;
  undefined/null failure outputs, clean scopes, GC and successful retry after OOM.

All four drivers fail against v0.473. Automatic execution may enter additional
native factory/target bodies: tests require a real native outer entry and compare
results, rather than assuming one total entry. These tests do not certify every
native Function's constructibility, all allocation mechanisms, concurrent context
access or the complete standard object model. Other CPU/OS acceptance and all nine
roadmap stages remain open.

References: Ruffle commit 2372a4e148a67d68efd796f4477b3cddd4f880cd,
[value.rs](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/value.rs),
[globals/object.rs](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/object.rs),
[globals/function.rs](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/function.rs),
and [parameters.rs](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/parameters.rs).

## Русский

Object(value), Function.call и Function.apply в загруженном SWF выбирают текущий
глобальный Number/Boolean/String через общий AVM1-механизм boxing. Сохранённая
регистрация или захваченный исходный класс больше не подменяют отсутствующий или
изменённый глобал. При невозможности boxing Object возвращает новый объект без
прототипа, а call/apply используют глобальный this. Getter и конструктор вызываются
по обычным правилам; ошибки/исключения передаются без вызова целевой функции.
__resolve намеренно пропускается, наследуемые свойства учитываются, регистр имён
зависит от версии SWF. Host-режим и прежние wire ID сохранены.

Проверены три примитивных типа, четырнадцать вариантов глобала, C API и четыре
Action-пути в трёх режимах, GC из getter-ов, жизнь нового receiver во время apply,
перенос адаптеров без библиотеки в приёмнике и отказы new/new[]. Все четыре теста
воспроизводят ошибки на v0.473. Для Object остаётся native-барьер коллектора;
в call/apply реальный GC проверяется. Полная совместимость стандартных объектов,
всех фабрик/аллокаторов, конкуренции и других CPU/ОС не заявлена; стадии открыты.

Validation v0.474: complete Linux minimal 930/930. AGG passed 935/949 in
the full run; 14 binaries could not launch after executable-mode resets.
After restoring their modes, the selected rerun passed 14/14: all 949 checks
have passing evidence across these runs. Four selected heap/global ASan+UBSan
checks passed, including the full allocation sweep. Stack ASan and LeakSanitizer
remain disabled. Generated logs and baseline binaries stay outside committed
source. No new foreign-OS/device or full foreign-CPU run is claimed.

Проверка v0.474: Linux minimal 930/930; все 949 AGG-проверок прошли в полном
прогоне и повторе 14 запусков после восстановления прав исполнения. Выбранные
heap/global ASan+UBSan — 4/4. Stack ASan/LeakSanitizer отключены; все стадии открыты.

## v0.477: prototype queries and native primitive results

Loaded `Object.prototype.isPrototypeOf(value)` now boxes a supplied primitive
through its current global constructor before traversing the resulting prototype
chain. This preserves getter/factory side effects, failures and exceptions. A
nullish or unavailable/nonobject result becomes a fresh bare object; omitted
arguments still return false directly. Unloaded host queries retain their
object-only behavior. The existing bounded prototype-cycle policy is unchanged.

[`fvm_exec_to_object`](../include/flashvm/execution.h#L628) accepts only object results from a boxing constructor.
Native NewObject/NewMethod may legitimately return primitives (v0.475), but
ToObject maps those results to undefined, allowing each consumer's existing
fallback policy. Native barriers remain active during isPrototypeOf; getter GC
attempts are checked as rejected rather than claimed as collection coverage.

В SWF `isPrototypeOf` преобразует примитивный аргумент через текущий глобальный
конструктор, сохраняя побочные эффекты и ошибки. Если объекта нет, создаётся
обычный объект без прототипа; без аргумента результат сразу false. Host без SWF
сохраняет прежнее поведение. ToObject отбрасывает примитив, возвращённый нативным
конструктором, хотя сам NewObject/NewMethod обязан сохранить такой результат.
Ограничение обхода циклов и native-барьер GC не изменены.

Tests / Проверки: object_prototype_global_boxing, to_object_native_primitive,
object_prototype_boxing_allocation. Current-global name lookup, inherited getters,
legacy spelling, resolver suppression, NUL strings, callback errors/exceptions,
interpreter/explicit/automatic JIT and allocation retries are covered.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_to_object` | function | [include/flashvm/execution.h:628](../include/flashvm/execution.h#L628) |
<!-- source-links:end -->
