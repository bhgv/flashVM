# Mode Tuning

Use [fvm_gc_mode](../../../include/flashvm/gc.h#L43) to select -100..100 and query [fvm_gc_threshold](../../../include/flashvm/gc.h#L87) to see the corresponding threshold.

**Русский:** Настройка частоты проверок GC и чтение порога.

<!-- example-catalog-api:start -->
## Назначение и особенности

Объясняет выбор режима GC в диапазоне -100..100 и чтение соответствующего порога. Полезна при настройке компромисса между частотой проверок и объёмом удерживаемой памяти; исходника main нет.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_gc_mode`](../../../include/flashvm/gc.h#L43) | [GARBAGE_COLLECTION.md#api](../../../doc/GARBAGE_COLLECTION.md#api) — Корни, pin/unpin, режимы и ручная сборка. |
| [`fvm_gc_threshold`](../../../include/flashvm/gc.h#L87) | [GARBAGE_COLLECTION.md#api](../../../doc/GARBAGE_COLLECTION.md#api) — Корни, pin/unpin, режимы и ручная сборка. |

[Описание в общем каталоге](../../README.md#example-embedding-mode_tuning).
<!-- example-catalog-api:end -->

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_gc_mode` | function | [include/flashvm/gc.h:43](../../../include/flashvm/gc.h#L43) |
| `fvm_gc_threshold` | function | [include/flashvm/gc.h:87](../../../include/flashvm/gc.h#L87) |
<!-- source-links:end -->
