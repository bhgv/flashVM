# Empty Object assignment / Пустое имя свойства — v0.145

Обычная запись Object в загруженном SWF игнорирует пустое имя после проверки
контекста/значения, до поиска флагов, setter, watcher и metatable hooks. SetMember
по-прежнему выполняет приведение имени к строке ровно один раз. Ранее созданное
в host-режиме пустое свойство остаётся читаемым, но загруженная запись его не
меняет. Counted имя из одного NUL непустое и остаётся допустимым.

object_empty_assignment проверяет незагруженный режим и SWF4..8, новые и ранее
существовавшие пустые свойства, приведение имени в SetMember и counted NUL.
Публичное создание accessor/watch с пустым именем уже запрещено API и отдельно
не моделируется тестом. SetVariable/local scope, полное приведение receiver,
версии функций и все стадии остаются открытыми.

## English

Loaded ordinary Object assignment ignores an empty name after context/value
validation, before flags, setters, watchers and metatable hooks. SetMember still
coerces its name exactly once. An empty host property created before loading
remains readable and unchanged by loaded assignments. A counted single NUL name
is nonempty and remains writable. Tests cover unloaded mode and SWF4..8, new
and preexisting empty properties, SetMember coercion and counted NUL. Public
empty accessor/watcher registration is already rejected and is not simulated by
this test. SetVariable/local scopes, receiver conversion, function versions and
all roadmap stages remain open.

## Follow-up v0.148 / Уточнение

Loaded watch now accepts empty names; an empty assignment still does not invoke
the registered callback. Загруженный watch теперь принимает пустое имя, но запись
пустого свойства по-прежнему не вызывает зарегистрированный callback.
