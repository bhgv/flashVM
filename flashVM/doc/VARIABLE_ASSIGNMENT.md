# Common variable assignment / Общий обработчик присваивания

v0.259 moves SetVariable destination selection and writing to the C89 value
service [`fvm_exec_variable_set`](../include/flashvm/execution.h#L542). Managed scope/target/this, a counted string name
and a value are explicit operands. The interpreter only validates/moves stack
operands and chooses its existing timeline/global fallback. Script activation
bindings are accessed through the managed chain rather than a raw local map.

Simple binding selection checks presence without reading its old value. Activation
nodes inspect their own slots; With nodes inspect their wrapped object and its
prototypes. An inherited With binding selects that receiver and blocks an outer
write. Setter/watch/property behavior is then delegated to the normal counted
property bridge. This fixes unwanted getter calls and getter exceptions while
selecting an assignment destination.

Qualified references split at the last dot/colon in every supported SWF version,
then resolve the owner with complete scoped-path retries. The final property may
be new; it is not read. Missing owner paths discard the write. A bare slash name
stays literal; empty names are ignored without root-array allocation. After a
slash, dots in clip names remain literal within the owner path. In particular,
GetVariable/SetVariable use one path split, while Delete2 additionally resolves
its owner as a variable reference, as documented in DELETE_REFERENCE.md.

[GC:] Scope, target, explicit this, full name, value, prefix/member and resolved
owner remain rooted across path getters and script setters. Trusted setter entry
keeps its rooted collector policy; outer native barriers are preserved. Callback
self-deletion/exceptions and allocation failures restore normal call/root metadata.
Prefix getter/setter side effects are not rolled back. Allocation injection with
ordinary data slots checks preservation of the old value until a successful write.
All root indices and the interpreter's SetVariable operand count use documented
uppercase header macros. Public API remains C89; implementation remains C++98.

Присваивание вынесено в общую C-службу. Поиск существующей привязки не вызывает
getter: проверяется наличие, затем выполняется обычная запись с setter/watch.
Поддержаны квалифицированные пути, внешние области, новые свойства, буквальные
имена со слешем и пустые имена. Значение и владелец удерживаются при GC в setter;
проверены исключения, самоудаление дескриптора и отказы памяти. Изменение не
закрывает историческую семантику записи this-cell, регистр имён и всю AVM1.

Remaining boundaries include mutable/inherited implicit-this cells, complete
legacy property casing, removed display targets and full external conformance.
All roadmap stages remain open; C services do not constitute a JIT backend.

Evidence: 485/485 Linux AGG and 485/485 Cairo tests. Two AGG execute-bit launch
failures passed after restoring permissions. Fifteen selected heap/global ASan +
UBSan checks passed; stack/leak coverage remains disabled. All 25 unchanged
reference SWFs matched in each profile. No SWF header or reference output changed.
Проверено по 485 тестов AGG/Cairo, 15 выбранных проверок памяти и 25 эталонов
в каждом профиле. Полнота совместимости этим не утверждается.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_variable_set` | function | [include/flashvm/execution.h:542](../include/flashvm/execution.h#L542) |
<!-- source-links:end -->
