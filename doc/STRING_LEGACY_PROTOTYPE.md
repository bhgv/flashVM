# String.prototype / Прототип String — v0.188

Загруженная библиотека создаёт String.prototype как ordinary Object без native
text/own length. valueOf/toString дают undefined, прочие String методы получают
обычный AVM1 type label. Untouched host prototype также преобразуется при
успешной загрузке SWF, сохраняя identity/пользовательские свойства. Все allocating
lookups выполняются до mutation; публикация SWF follows без выделений/callbacks.
Ошибочная загрузка не меняет прототип. Явно заменённые/customized length прототипы
не преобразуются автоматически. Validator принимает оба поддерживаемых layouts.
ABI/graph format не меняются, generic Object layout уже поддерживается.

string_legacy_prototype проверяет SWF4..8, install-before/after-load, invalid SWF,
identity/NUL marker, отсутствие length, value methods/casing, traced GC/queue/GC,
constructor alias и создание нового String восстановленным constructor.
Полные customized/boxed overrides, host-instance-before-load rules, property
case, native GC callback entry и все стадии остаются открытыми.

## English

Loaded String.prototype is an ordinary Object without native text/own length.
Value methods return undefined; other String methods use normal AVM1 type-label
coercion. Successful SWF loading converts an untouched host prototype while
preserving identity/user properties. All allocating lookups precede mutation;
SWF publication follows without allocation/callbacks. Invalid loads preserve the
prototype. Explicitly replaced/customized-length prototypes are not automatically
converted. Validation accepts both layouts; existing generic graph support needs
no wire/ABI change.

Tests cover SWF4..8 installation before/after loading, invalid SWFs, identity/NUL
markers, missing length, value/case methods, traced GC/queue/GC, constructor alias
and construction through a restored class. Customized/boxed overrides, pre-load
host instances, property case, arbitrary native callback GC and all stages remain
open. Reference: pinned DeclContext.build_class creates new_without_proto and
String.create_class adds methods without invoking the String constructor.
