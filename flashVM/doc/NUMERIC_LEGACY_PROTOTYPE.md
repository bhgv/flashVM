# Numeric prototypes / Прототипы Number и Boolean — v0.196

В загруженных SWF4–8 Number.prototype и Boolean.prototype являются обычными
объектами без boxed primitive. Их native valueOf/toString возвращают undefined.
При установке стандартной библиотеки после загрузки сразу создаётся этот layout.
При загрузке после установки неизменённые host прототипы переходят в новый
layout с сохранением identity, методов, свойств и constructor alias.

Number/Boolean подготавливаются до активации String. Все allocating lookups
предшествуют первому изменению; после активации String commit остальных типов
не выделяет память и не вызывает callback. При OOM остаются прежние прототипы
и состояние SWF; повторная загрузка работает. Изменённые native layouts или
заменённые прототипы не активируются. Старые boxed экземпляры сохраняют данные.

## English

Loaded Number.prototype/Boolean.prototype are ordinary objects with no boxed
primitive; native valueOf/toString return undefined. Installation after loading
creates this layout directly. Loading after installation activates unchanged
host prototypes while retaining identity, methods, properties and class aliases.

Number/Boolean preparation precedes String activation: all allocating lookups
finish before any mutation, and remaining commits cannot allocate or call user
code. OOM preserves every old prototype and SWF state; retry succeeds. Replaced
or customized native layouts are skipped. Existing boxed instances retain data.
Validation accepts both layouts; native IDs and graph/API versions are unchanged.

numeric_legacy_prototype covers SWF4–8 and both installation orders, invalid
loads, identity/NUL markers, old instances, traced GC/channel/GC, restored class
construction and prototype alias. Expanded string_legacy_load_allocation sweeps
C++ allocation failures over decoding and joint primitive activation with
balanced allocations. C malloc failures and customized-layout acceptance remain
open. Reference: pinned Ruffle property_decl.rs build_class and number.rs/
boolean.rs create_class, commit 2372a4e148a67d68efd796f4477b3cddd4f880cd.
Array/other prototype layouts and all roadmap stages remain open.
