# Readonly watcher order / Порядок readonly watcher — v0.149

Загруженная обычная/индексная запись вызывает watcher до проверки текущего
READ_ONLY. Callback может снять флаг для разрешения записи или установить его
для запрета. Script setter по-прежнему вызывается на readonly accessor, но
retained data защищены. Индексный путь передаёт подготовленное значение setter
без повторного watcher вызова. Незагруженная проверка до watcher не меняется.

[GC:] Выбранный prototype lookup owner удерживается allocation-free scope во
время callback, который может отсоединить его. Частное prepared_watch значение
не сериализуется и не меняет публичный C89 ABI.

object_readonly_watch проверяет SWF6..8 и незагруженный режим: обычные/numeric/
counted NUL ключи, virtual setter, снятие/установка/сохранение флага, ровно один
watcher, raw data, восстановление lookup/setter/watch корней и GC после записи.
Native Array length/index guard, ошибки watcher с последующей setter-записью,
case/version и все стадии остаются открытыми; callback эффекты не откатываются.

## English

Loaded ordinary/indexed assignment calls watchers before reading current own
readonly flags. Callbacks may clear or set the flag to permit or suppress data
storage. Script setters still run on readonly accessors while retained data stays
protected. Indexed dispatch passes the prepared value without invoking watchers
twice. Unloaded pre-watcher checks remain unchanged.
The selected prototype lookup owner is rooted through callback detachment with
an allocation-free scope. The private prepared_watch argument does not change
public C89 ABI or serialization. Tests cover SWF6..8/unloaded mode, ordinary,
numeric/countеd NUL keys, virtual setters, flag changes, one callback, raw data,
root/metadata unwind and post-write GC. Native Array guards, watcher exception
ordering, case/version and every stage remain open; callback effects are not
rolled back.
