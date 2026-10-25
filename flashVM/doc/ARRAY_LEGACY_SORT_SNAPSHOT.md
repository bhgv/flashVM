# Loaded sort snapshot / Снимок сортировки — v0.162

Загруженный sort/sortOn захватывает все собственные raw значения до
преобразования первого ключа. Пустые позиции обычного native Array становятся
собственными undefined и участвуют в RETURNINDEXEDARRAY. Непрерывный снимок
[FVM_Value](../include/flashvm/value.h#L26) удерживает объекты, метатаблицы и строки через временный GC frame;
возврат, ошибка callback и исключение выделения восстанавливают цепочку корней.
Копирование входа ограничено существующим пределом 65536; обработка ключей
учитывается в общем instruction budget. Незагруженный sparse контракт сохранён.

array_legacy_sort_snapshot проверяет SWF4..8, toString с очисткой receiver,
сохранение всех исходных объектов, наличие полного снимка в цепочке корней,
восстановление depth/roots после ошибки и собственные undefined в native holes.
После успешной сортировки выполняется реальная сборка мусора. Сборка внутри
произвольного native callback остаётся ограничена существующим native barrier;
этот тест не заявляет такую сборку поддержанной. Callback effects при ошибке
не откатываются. Точный порядок/coercion и UNIQUESORT остаются открытыми.

## English

Loaded sort/sortOn captures every own raw value before the first key conversion.
Native Array holes become own undefined values and participate in indexed return.
A contiguous [FVM_Value](../include/flashvm/value.h#L26) snapshot roots objects, metatables and strings through a
temporary collector frame. Return, callback errors and allocation exceptions
restore caller roots. Existing 65536 limits apply; key preparation shares the
instruction budget. Unloaded sparse behavior is preserved.

Tests cover SWF4..8, receiver clearing from toString, original object retention,
complete snapshot roots, depth/root restoration on errors and own undefined
native holes. Actual GC runs after successful sort. Arbitrary native callbacks
remain subject to the existing collection barrier; in-callback GC is not claimed.
Callback effects are not rolled back on failure. Exact comparison/coercion order,
custom UNIQUESORT and all roadmap stages remain open. No ABI/wire changes.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../include/flashvm/value.h#L26) |
<!-- source-links:end -->
