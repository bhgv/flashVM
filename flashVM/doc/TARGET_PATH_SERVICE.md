# TargetPath service / Путь отображаемого объекта — v0.292

Action TargetPath and C89 `fvm_exec_target_path(context, input, output)` now share
one handler. It boxes the value through the current primitive constructor, or
creates a bare object when boxing yields no object. Ordinary objects and the
MovieClip class prototype return undefined. A real native MovieClip returns its
counted dot path, for example `_level0.child`. A string containing that path is
an input value to box, not a display-path lookup request.

[GC:] Original and prepared values stay rooted during constructor callbacks and
path copying. Exceptions propagate; output may alias input and remains unchanged
on failure. Constructor side effects are retained. Loaded empty stacks supply
undefined; unloaded empty stacks retain the strict error policy.

Path production reuses the native scene model and bypasses replaceable toString
and _target callbacks. Embedded NUL names remain counted. The existing parent
walk bound and level-prefix buffer are named in movieclip_path_constants.h.
No resource kind or graph schema is added. Other display-object kinds, removed
clip semantics and complete scene compatibility remain open.

Primary comparison: pinned Ruffle
[action_target_path](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs).
C89, Action, collecting/throwing constructor and allocation fixtures exercise
SWF4–8, root/prototype/ordinary values, primitive boxing, counted path names,
output aliasing, empty stacks and failed construction/copying. Existing native
path tests retain cycle and transport coverage.

Добавлен общий C-интерфейс и Action TargetPath. Упаковка примитивов сохраняет
вызовы конструкторов, GC и исключения; строка с путём не разрешается как адрес
объекта. Для MovieClip путь берётся из модели сцены без пользовательского
преобразования в строку, для обычного объекта результат — undefined. Проверены
имена со встроенным NUL и сохранение входа/выхода при отказе памяти. Другие типы
отображаемых объектов, удалённые клипы и полная совместимость остаются открытыми.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_target_path` | function | [include/flashvm/execution.h:209](../include/flashvm/execution.h#L209) |
<!-- source-links:end -->
