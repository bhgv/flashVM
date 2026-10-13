# Readonly setter в SWF / Loaded readonly setter — v0.141

Загруженный SWF вызывает function setter даже у READ_ONLY script descriptor.
Само retained data не меняется, пока собственный флаг READ_ONLY установлен.
Если setter снимает флаг, subsequent data-запись разрешена: flags читаются после
callback. Унаследованное readonly data не запрещает receiver создать собственное
свойство. Нативные/обычные собственные readonly данные по-прежнему защищены.
Незагруженный C-режим сохраняет прежние строгие проверки до callback.

Setter scope теперь владеет immutable counted ключом. Его копия может вернуть
OOM до callback; owner/name метаданные публикуются только после успешной копии,
частично созданные корни восстанавливаются автоматически. Три object-root
кадра по-прежнему allocation-free. Ключ остаётся действительным при удалении
свойства внутри callback и восстанавливается после вложенных setter/ошибок.
Приватное поле контекста не сериализуется; ABI C89 и структура графа не меняются.

object_readonly_setter проверяет SWF6..8 и незагруженный C-режим: numeric/countеd
NUL accessor, вызов readonly setter без data-изменения, снятие readonly внутри
callback, отсутствие запрета собственному shadow по readonly data прототипа,
восстановление owner/name/корней. Правила сверены с pinned set_internal,
set_local и Property::set_data.

Readonly watcher порядок, самозамена/удаление setter и последующая data-запись,
Array index/length virtual guard, версии и все стадии остаются открытыми.

## English

Loaded SWF executes function setters on readonly script descriptors, but retained
data stays unchanged while current own READ_ONLY remains set. A setter clearing
its flag enables subsequent data storage; flags are checked after execution.
Inherited readonly stored data does not prevent an own receiver shadow. Ordinary
own readonly data/native fields remain protected. Unloaded C mode keeps strict
pre-callback checks.

Setter scope now owns an immutable counted key. Copy failure occurs before
owner/name metadata changes, with automatic partial root unwind. Its three
object-root frames still allocate nothing. Name remains valid after callback
property deletion and is restored through nesting/errors. Context metadata is
not serialized; C89 ABI/wire structure stays unchanged. The test covers SWF6..8,
unloaded mode, numeric/countеd NUL keys, readonly callback with unchanged data,
callback flag clearing, inherited data shadowing and metadata/root restoration.
Readonly watcher order, self-detachment/replacement data storage, real Array
virtual guards, version rules and every stage remain open.
