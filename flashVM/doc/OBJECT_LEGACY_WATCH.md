# Loaded Object.watch / Watcher в SWF — v0.148

watch/unwatch приводят загруженное имя через AVM1 string bridge. watch делает
это до проверки callback, принимает пустое имя и virtual properties. Запись
пустого свойства по-прежнему игнорируется, поэтому его watcher не вызывается.
Незагруженные правила регистрации сохраняются. Native MovieClip fields остаются
за существующим ограничением модели, без заявления полного stage-object watch.

Старое значение watcher читается по own/prototype stored data без getter и
metatable hooks. Script virtual descriptor предоставляет retained data;
нативный Array length предоставляет сохранённую длину. Builtin method handles
материализуются без вызова callback. Нативные C accessor без модели retained data
дают undefined. Предел обхода прототипов остаётся 1024, не полная AVM1 рекурсия.
Readonly watcher order, exception ordering и case/version rules открыты.

object_legacy_watch проверяет SWF6..8, numeric/object name, unwatch/coercion до
валидации callback, virtual retained data без getter, empty и counted NUL,
данные после GC и watcher+унаследованный setter без изменения prototype data.
Существующие GC/queue watch тесты остаются частью проверки. ABI/wire не меняются.

## English

Loaded watch/unwatch coerce names using the AVM1 string bridge. Watch converts
before callback validation, accepts empty keys and virtual properties. Empty
assignment still skips the registered watcher. Unloaded registration remains
unchanged; native MovieClip fields keep existing model restrictions.
Watcher old values traverse own/prototype stored data without getters or
metatable hooks. Script virtual descriptors expose retained data; native Array
length exposes stored length. Lazy builtin method handles are materialized
without callback invocation. Native C accessors without retained data yield
undefined. Prototype limit remains 1024; readonly watcher/exception ordering,
case/version rules and all stages remain open.
The SWF6..8 test covers numeric/object name conversion, unwatch, conversion before
callback validation, virtual data without getters, empty/countеd NUL, data after
GC and inherited setters preserving prototype data. Existing GC/queue watch
coverage stays in the suite. No C89 ABI or wire format changes.
