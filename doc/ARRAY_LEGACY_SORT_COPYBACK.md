# Loaded sort copyback / Запись сортировки — v0.166

Загруженный native sort/sortOn последовательно записывает только захваченный
диапазон через raw element storage. Текущая length и хвост receiver сохраняются:
если comparator добавил индекс 7, он не удаляется при записи первых четырёх
значений. Native рост length выполняется до проверки flags данных; READ_ONLY
length блокирует изменение длины, но не появление own slots за её пределами.
Index setter/watcher не вызываются. Незагруженный staged commit сохраняется.

array_legacy_sort_comparator проверяет SWF4..8: callback расширяет length и
добавляет хвост; сокращает length до 1/0 и ставит READ_ONLY; возвращает ошибку
после расширения; использует RETURNINDEXEDARRAY без обратной записи. Root/depth
восстановлены, захваченная длина index result остаётся 4. Ошибка callback не
публикует sorted snapshot, но её изменения receiver сохраняются. Ошибка
выделения/бюджета во время raw copyback может оставить частичную запись,
как последовательный путь эталона; транзакционность loaded sort не заявлена.
Полные flags/argument overloads, native GC barriers и все стадии открыты.

## English

Loaded native sort/sortOn writes only the captured range with sequential raw
element stores. The receiver's current length and tail survive callback changes.
Native length growth precedes data flags; readonly length prevents length updates
while still permitting own slots outside that length. Index setters/watchers do
not run. Unloaded staged commit is preserved.

Tests cover SWF4..8, callback-created tails, readonly length shrinking to one/zero,
errors after growth, indexed return without copyback and restored roots/depth.
Indexed results keep the captured length of four. Comparator failure does not
commit sorted values, but callback effects remain. Allocation/budget failure
during sequential copyback may leave partial writes, following reference ordering;
loaded atomicity is not claimed. Full flag/argument cases, native GC barriers and
all roadmap stages remain open. No public ABI/wire changes.
