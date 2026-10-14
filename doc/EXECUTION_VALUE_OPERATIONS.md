# Shared value operations / Общие операции со значениями

v0.242 moves own-member deletion, legacy numeric comparison and logical
operations into separate handler files. Each logical service includes its
supported Flash-version variants. Public C89 callers pass semantic operation
identifiers and values, without Action IDs, bytecode or operand stacks.

`fvm_exec_legacy_compare` preserves right-before-left number conversion for
legacy equality and left-before-right conversion for legacy ordering. Existing
comparison metamethod lookup/result projection remains unchanged. This service
is distinct from dynamic Equals2/Less2 and their primitive/string rules.

`fvm_exec_logical` supplies NOT/AND/OR. String truth follows the loaded SWF policy;
both binary operands are converted left to right. Unary NOT accepts a NULL right
operand. All successful operations return Boolean values.

`fvm_exec_delete_member` converts the property name, deletes own members subject
to property flags and reports false for absent/inherited properties and super.
It preserves the interpreter's existing primitive-owner policy. Delete2/scoped
variable deletion and full historical casing remain open work.

[GC:] Documented uppercase macros identify operand roots and the converted-name
slot. Operands remain rooted across prepared native callbacks that collect or
throw. Outputs may alias inputs and remain unchanged on errors. Allocation
injection checks failure containment and root-frame restoration.

Удаление свойства, старое числовое сравнение и логические операции вынесены
в отдельные C-службы для интерпретатора и будущего JIT. Сохранены порядок
преобразования, версионные правила истинности строк и атрибуты удаления.
Проверяются C89-вызовы, встроенный NUL в имени, наследуемые свойства,
неудаляемые свойства, алиасы результата, GC, исключения и отказы памяти.
Удаление переменных Delete2 и полная совместимость AVM1 ещё не завершены.

Evidence: Linux AGG and Cairo each pass 449 tests; nine AGG execute-bit launch
failures pass on retry after permissions are restored. Seven focused heap/global
ASan+UBSan checks pass, with no stack/leak instrumentation in this environment.
All 23 unchanged original SWFs retain exact trace output in both profiles.
