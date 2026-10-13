# Флаги замены addProperty / Virtual replacement flags — v0.139

Замена существующего собственного data/accessor/method свойства сохраняет его
flags. Null setter не добавляет READ_ONLY существующему writable свойству;
новое свойство с null setter по-прежнему READ_ONLY. Замена readonly свойства
функцией-setter не снимает READ_ONLY. DONT_DELETE запрещает удаление, но больше
не запрещает addProperty заменить descriptor. Прежние data сохраняются.

Null setter представлен внутренним script bridge: для writable существующего
свойства обычная запись обновляет retained data без вызова функции; новые
readonly свойства блокируются прежней проверкой. C89 ABI/структура wire не
меняются. Тест object_add_property_flags проверяет flags 3/4, защищённую замену,
сохранение data, writable null setter, counted NUL и GC. Прежние names и
allocation тесты исправлены по правилу сохранения flags; names явно снимает
READ_ONLY перед записью, когда нужен вызов setter.

Для настоящих Array index/length действует прежний guard. Вызов setter на
readonly script descriptor, inherited setter data, исходные lazy-method data,
version flags и все стадии остаются открытыми. Строгая C-проверка READ_ONLY
не объявляется полной исторической семантикой callback для загруженного SWF.

## English

Replacing an existing own data/accessor/method property preserves flags. Null
setters make only new properties READ_ONLY; existing writable properties stay
writable, and replacing a readonly setter does not clear its flag. DONT_DELETE
prevents deletion rather than descriptor replacement. Prior data stays retained.

A null setter uses the internal script bridge so writable existing descriptors
update data without executing a function. New readonly descriptors remain
blocked by existing checks. ABI/wire structure is unchanged. The flags test
covers protected replacement, retained data, writable null setters, flags 3/4,
counted NUL and GC. Existing name/allocation expectations now follow preservation;
the name test explicitly clears readonly before testing setter execution.
Real Array index/length guards, readonly script setter callbacks, inherited
setter data, lazy method data/version flags and all roadmap stages remain open.
