# Borrowed sortOn preparation / Подготовка заимствованного sortOn — v0.177

Загруженный sortOn подготавливает поля/options до захвата receiver, включая
заимствованный метод. Синхронный внутренний callback получает уже скопированные
имена/flags, повторные преобразования не выполняются. Копия снимается с длины
и собственных данных receiver после всех преобразований. Поэтому изменение
элементов/длины из toString/valueOf участвует в текущей сортировке. Fallback
для не-Array Object поля также работает на заимствованном receiver.
Внутренний metadata pointer не сохраняется; SortSpec не содержит managed edges.
Публичные C API и незагруженная семантика не меняются.

array_legacy_sort_on_borrowed_prepare проверяет SWF4..8, Object/Array receivers,
расширение длины с 2 до 3 в поле, изменения данных в поле и option, каждый callback
ровно один раз, ошибки в обеих фазах с сохранением изменений без sorted copyback,
traced GC после выполнения и восстановление root/depth/setter state.

## English

Loaded sortOn prepares fields/options before capturing native or borrowed receiver
data. A synchronous private fast callback consumes copied names/flags without
repeating coercion. Length and own-data snapshots are taken after all conversions,
so toString/valueOf mutations participate in the current sort. Non-Array Object
field fallback also supports borrowed receivers. The internal metadata pointer is
not retained; SortSpec has no managed edges. Public C APIs/unloaded semantics
are unchanged.

Tests cover SWF4..8, Object/Array receivers, field growth from length 2 to 3,
field/option data mutations, exactly one call per conversion, errors in both
phases retaining side effects without sorted copyback, post-call traced GC and
restored root/depth/setter state. Full Array/AVM1 semantics, arbitrary native
callback GC and all roadmap stages remain open. Reference: pinned sort_on calls
sort_internal only after fields/options conversion.
