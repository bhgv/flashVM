# registerClass names / Имена зарегистрированных классов — v0.150

registerClass проверяет constructor до приведения имени, поэтому неправильный
constructor возвращает false без name callback. Загруженный SWF использует
общий AVM1 string bridge и принимает пустой символ. undefined имя становится
пустым в SWF4..6 и "undefined" в SWF7..8; незагруженное преобразование прежнее,
пустое host имя по-прежнему отклоняется. NULL/undefined constructor удаляет запись.

object_register_class_names проверяет незагруженный режим и SWF4..8: порядок
валидации/приведения, empty/undefined/countеd NUL имена, retention после GC,
queue transfer Object class вместе с registry и вызов восстановленного AVM1
constructor без standard-install в принимающем контексте. Existing sprite/scene
и registry transfer остаются обязательной проверкой. ABI/wire не меняются.
Case-insensitive отдельные среды, function/base-clip версии, typed serialization,
полная привязка symbols и все стадии остаются открытыми.

## English

registerClass validates constructor before coercing its name, rejecting invalid
constructors without name callbacks. Loaded names use the AVM1 string bridge
and may be empty; undefined maps to empty in SWF4..6 and "undefined" in SWF7..8.
Unloaded conversion/empty rejection remains unchanged. Null/undefined constructor
removes the entry. Tests cover unloaded mode/SWF4..8, validation order, empty,
undefined/countеd NUL, GC, queue transport of class+registry and restored AVM1
constructor invocation without installing standard globals at the receiver.
Existing scene/sprite and registry transport coverage remains required. No ABI
or wire changes. Separate case-insensitive environments, function/base-clip
versions, typed serialization, full symbol binding and all stages remain open.
