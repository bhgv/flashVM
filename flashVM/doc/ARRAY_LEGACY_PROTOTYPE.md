# Array prototype / Прототип Array — v0.197

В загруженных SWF4–8 Array.prototype — обычный Object без native length.
Установка после загрузки создаёт этот layout сразу. Загрузка после установки
активирует неизменённый пустой native прототип, сохраняя identity, методы,
пользовательские свойства, constructor alias и cached builtin result prototype.
Существующие массивы сохраняют собственный native length и данные.

Подготовка Array выполняется вместе с Number/Boolean до изменения String.
В подготовке сохраняются адреса штатных ключей length; commit удаляет accessor
и attributes без allocating lookup. Отказы C++ allocation не изменяют ни один
прототип и позволяют повторить загрузку. Изменённые длина/индексы/descriptor
не активируются. Форматы графа и C API не менялись.

## English

Loaded Array.prototype is an ordinary Object with no native length. Installing
after load creates this layout; loading after install activates an unchanged
empty native prototype while preserving identity, methods, properties, class
aliases and the cached builtin result prototype. Existing arrays retain their
own native length and data. Generic methods can subsequently create ordinary
length/index properties on the prototype without making it a native Array.

Array preparation runs before any primitive activation. Retained native length
keys allow allocation-free accessor/attribute removal after String commit.
C++ allocation failure preserves all layouts and permits retry; customized
length/index/descriptor shapes are skipped. Wire/API versions are unchanged.

array_legacy_prototype covers SWF4–8/both installation orders, invalid loads,
identity/NUL marker, old instances, cached slice result prototype, GC/channel/GC,
generic push with ordinary writable length and restored class construction.
Expanded string_legacy_load_allocation checks joint Array/Number/Boolean/String
OOM recovery and allocation balance. Reference: pinned Ruffle array.rs
create_class and property_decl.rs build_class, commit
2372a4e148a67d68efd796f4477b3cddd4f880cd. C malloc failure coverage, customized
layouts, other standard prototypes and all roadmap stages remain open.
