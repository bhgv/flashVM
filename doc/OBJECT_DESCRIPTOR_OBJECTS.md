# Объекты getter/setter / Descriptor Objects — v0.130

Object.addProperty принимает любой Object как getter, любой Object или null
как setter. Если сохранённый Object не имеет вызываемой функции, чтение даёт
undefined, а запись игнорируется без ошибки вызова. Смешанные дескрипторы
сохраняют вызываемую сторону: функция-getter с обычным setter-Object читается;
обычный getter-Object с функцией-setter вызывает setter. Null setter сохраняет
READ_ONLY. Примитивы вместо getter или setter по-прежнему отклоняются.

object_add_property_objects проверяет эти комбинации, флаги, отсутствие
превращения дескриптора в data при записи, GC и перенос через очередь в новый
контекст с GC перед повторным чтением/записью. Обычные descriptor-Object остаются
закреплены скрытым объектом дескриптора. Проверка не меняет C89 ABI или формат
графа: используются существующие корни/accessor и идентификаторы callbacks.
Правила сопоставлены с [pinned addProperty](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/object.rs)
и call_getter/call_setter в avm1/object/script_object.rs.

Замена защищённых свойств, числовые/length accessor настоящих Array, версия
дескрипторов и полная сверка прототипов остаются открытыми. Все стадии открыты.

## English

addProperty accepts any Object as getter and any Object or null as setter.
A non-function getter returns undefined; a non-function setter ignores writes
without a call error. Mixed descriptors retain their executable side. Null
setters keep READ_ONLY, and primitive descriptor operands remain invalid.

object_add_property_objects covers combinations, flags, descriptor retention
on write, GC and queue transfer followed by GC/read/write in a new context.
Existing descriptor capture roots and callback identities suffice; there is no
C89 ABI or wire-format change. Rules follow the pinned primary implementation.
Protected replacement, real Array index/length accessors, descriptor-version
rules and complete prototype conformance remain open, as do all roadmap stages.
