# Mutable this cells / Присваивание ячейке this

v0.515: SetVariable with the exact counted name this in loaded SWF5–8 replaces
the active activation's typed this value. Subsequent reads and qualified paths
observe that value. Undefined, Null, Boolean, Number, counted String, Object and
primitive metatables are retained; primitive this is not boxed for path traversal.
The case variant THIS remains an ordinary assignment. Loaded SWF4 and unloaded
host assignment policies are unchanged. Direct execution helpers without an
active script cell retain ordinary scope/property assignment behavior.

В SWF5–8 SetVariable с точным именем this заменяет значение активной ячейки.
Последующие чтения и составные пути используют новое значение. Поддержаны
Undefined, Null, Boolean, Number, строки с NUL, Object и примитивные метатаблицы.
Обход пути не упаковывает примитивный this. Присваивание имени THIS остаётся
обычным присваиванием. Поведение SWF4, host без SWF и helper без активной
скриптовой ячейки сохранено.

Each activation uses an allocation-free stack cell. The collector traverses
active and suspended cells, including counted strings and metadata. Borrowed
string assignment owns its bytes before publication; OOM leaves the old cell
unchanged. Function2 this flags copy the caller's full typed value, independently
of the actual receiver/preload. With and Catch copy this; Try and Finally share
the enclosing cell. Catch binding named this changes only its copied cell.
Interpreter and generated-body scopes restore the caller on all exits.

Ячейки лежат в стеке без отдельной аллокации. GC обходит активные и приостановленные
ячейки, включая строки и метатаблицы. Заимствованная строка копируется до публикации;
при OOM прежняя ячейка сохраняется. Флаги this у Function2 копируют полное значение
вызывающего независимо от actual receiver/preload. With/Catch копируют ячейку,
Try/Finally используют родительскую. Состояние восстанавливается при любом выходе.

The public ABI and graph format are unchanged. FVM_ScriptFrame.implicit_this is
an object-valued entry snapshot, null for primitive cells; live reads/writes use
execution helpers. Active stack cells are not serialized; transported pure AVM1
functions create destination-local cells. Super/preload values still derive from
the actual receiver at activation entry, not later this assignment.
ABI и формат графа не изменены; активные стековые ячейки не сериализуются.

Primary comparison: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
avm1/activation.rs set_variable, this_cell, with_new_scope, action_with/action_try;
avm1/function.rs ActionFunction::exec; avm1/value.rs as_object. This is source
comparison, not an original Flash Player oracle. Native receiver coercion and
complete this/scope/AVM1/SWF compatibility remain open; all nine stages remain open.
Сравнение исходников не заменяет оригинальный Flash Player. Преобразование нативного
получателя и полная совместимость областей видимости/AVM1/SWF ещё открыты.

Tests: this_cell_assignment covers value types, metadata, paths, Function2 and
SWF4 controls; this_cell_assignment_roots deletes bindings, mutates a nested cell
from borrowed NUL bytes, collects and checks parent/capture roots and errors.
this_cell_assignment_allocation checks atomic replacement under allocation
failures and retry. this_cell_assignment_transport checks function/value graph
restoration. this_cell_blocks checks With/Try/Catch/Finally, including catch this,
under interpreter, explicit and automatic native JIT execution.

Validation / Проверка: Linux x64 C++98 minimal 1059, AGG 1078 and Cairo 1079
cases passed, with execute-permission repairs and failed-case reruns on 8/15/14
unique generated binaries (four AGG cases needed an additional permission rerun).
Initial Cairo/sanitizer links used stale header dependency records and required
second dependency-aware builds. Seven targeted ASan/UBSan cases and 26 Unicorn
branch/error cases on x86, ARM, ARM64, RV32 and RV64 passed; native x64 ran in
host suites. A fresh GNU readelf audit passed for 1068 Cairo CTest ELF artifacts,
with only libc, libgcc_s, libm and libstdc++ in direct DT_NEEDED. Foreign OS,
transitive/dlopen dependencies and complete roadmap acceptance remain open.
