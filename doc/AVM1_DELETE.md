# ActionDelete / Удаление свойства

v0.89 реализует ActionDelete (0x3a): снимает объект и имя со стека,
преобразует имя в строку и удаляет только собственное свойство. Возвращает
boolean; отсутствующее, наследуемое, DONT_DELETE или свойство super дают false.
Имя сохраняет NUL и каноническую UTF-16 идентичность. Getter не вызывается.
Удаление индекса массива не меняет length; защищённый length не удаляется.
Ошибка преобразования распространяется до изменения стека.

ActionDelete converts its top operand to a complete counted property name and
deletes an own member from the owner below it. Missing/inherited members,
DONT_DELETE and super facades return false. Indexed deletion preserves Array
length; own accessor deletion does not invoke its getter. Both operands remain
GC roots throughout name coercion and deletion. Name conversion errors propagate.

Regression avm1_delete covers missing/inherited properties, flags, accessors,
NUL and equivalent surrogate keys, numeric Array indices, length protection,
name callback/error handling, primitive owners and stack underflow.
Reference cross-check: [pinned Ruffle action](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs)
and [own-property deletion](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/script_object.rs).
These are implementation comparisons, not a captured player oracle.

ActionDelete2, version-dependent case-insensitive names and complete scope/path
semantics remain open; this does not complete stage 2. Удаление переменной,
исторический регистр имён и полные правила областей видимости пока открыты.
