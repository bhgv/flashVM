# Loaded method names / Имена методов в загруженных SWF

v0.241 applies the existing version-aware string conversion to loaded CallMethod
and NewMethod names. Only undefined bypasses conversion and selects the anonymous
call. Null becomes "null", numbers become text and booleans follow the loaded
SWF version. Empty resulting text also selects the anonymous call. Object names
can execute toString; errors/exceptions stop before method dispatch and preserve
the C output. Unloaded host contexts retain their previous restricted contract.

Null/undefined receivers now produce undefined for loaded NewMethod as well as
CallMethod, without converting the name. A loaded constructor member that is not
an object also produces undefined. Unloaded host construction remains strict.
Argument-count conversion, anonymous this policy, full property casing and
missing/noncallable member behavior remain separate open compatibility work.

[GC:] The primitive box is rooted before converting the name. Converted text
has its own documented root slot; inputs, arguments, receiver and constructor
remain retained through nested callbacks. The name callback fixture collects
inside an attached prepared native body and raises an exception on a second call.

Source comparison: unchanged Ruffle commit
`2372a4e148a67d68efd796f4477b3cddd4f880cd`, `core/src/avm1/activation.rs`,
`action_call_method` and `action_new_method`. This is a primary implementation
comparison, not an independently executed Flash Player oracle for the new cases.
Existing original SWFs and their output oracles are never rewritten.

Для загруженных SWF имя метода теперь преобразуется в строку: null означает
свойство «null», числовые и логические значения следуют правилам версии SWF.
Undefined и пустая строка выбирают анонимный вызов. Преобразование объекта
может вызвать toString; исключение сохраняет прежний результат C-вызова и
останавливает диспетчеризацию. Упакованный примитив и полученная строка
защищены отдельными корнями GC. Для NewMethod с отсутствующим получателем
или не-объектным конструктором возвращается undefined. Хостовый режим без
загруженного SWF сохраняет прежний строгий контракт. Это частичная проверка
семантики вызовов; все стадии общего роудмапа остаются открытыми.

Evidence: 446 tests pass in each Linux AGG/Cairo profile after restoring execute
bits for three AGG and one Cairo launch failures. Six focused heap/global
ASan+UBSan checks pass; stack/leak instrumentation remains unavailable. All 23
unchanged original SWFs retain byte-exact output in both profiles.
