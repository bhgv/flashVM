# Loaded push / Push в SWF — v0.153

Загруженный push читает own signed length, записывает элементы через raw data
без удаления существующего descriptor и без getter/setter/watcher. Retained data
и длина обычного Object меняются напрямую с учётом собственных readonly flags.
Native Array length обновляется только при разрешении её readonly флагом;
создание raw индекса за readonly length при этом допускается. Return value —
вычисленная новая длина. Незагруженный путь сохраняется.

array_legacy_push проверяет SWF6..8: virtual length/index raw data, отсутствие
callbacks/watch, readonly length и index, counted NUL после GC и нативный Array
с readonly length и новым own индексом за её пределами. Общий work budget
включает приведение length и каждую запись. Отрицательная signed length и новая
длина выше INT32_MAX остаются явными UNSUPPORTED; native C accessor data,
case/version и все стадии открыты. Ошибки не откатывают начатые изменения.

## English

Loaded push reads own signed length and writes raw element data without deleting
existing descriptors or invoking getters/setters/watchers. Retained data and
borrowed length are updated directly subject to own readonly flags. Native
readonly length remains unchanged while raw indices beyond it may be created.
The computed new length is returned; unloaded behavior stays unchanged.
SWF6..8 tests cover virtual length/index data, no callbacks/watchers, readonly
length/index, counted NUL after GC and native readonly length with an outside
own slot. Shared budget includes length coercion and writes. Negative signed
lengths/new length above INT32_MAX remain UNSUPPORTED; native C accessor data,
case/version and all stages stay open. Failed mutations are not rolled back.
