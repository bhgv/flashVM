# Manual Collection

Pin externally retained objects, call [fvm_gc_collect](../../../include/flashvm/gc.h#L54) at a safe point, then unpin.

**Русский:** Закрепление объектов C и ручная сборка в безопасной точке.

<!-- example-catalog-api:start -->
## Назначение и особенности

Краткая заметка о ручном запуске GC в безопасной точке и закреплении объектов, которые удерживает хост. Это последовательность действий для интеграции, а не отдельная исполняемая программа.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_gc_collect`](../../../include/flashvm/gc.h#L54) | [GARBAGE_COLLECTION.md#api](../../../doc/GARBAGE_COLLECTION.md#api) — Корни, pin/unpin, режимы и ручная сборка. |
| [`fvm_gc_pin`](../../../include/flashvm/gc.h#L60) | [GARBAGE_COLLECTION.md#api](../../../doc/GARBAGE_COLLECTION.md#api) — Корни, pin/unpin, режимы и ручная сборка. |
| [`fvm_gc_unpin`](../../../include/flashvm/gc.h#L70) | [GARBAGE_COLLECTION.md#api](../../../doc/GARBAGE_COLLECTION.md#api) — Корни, pin/unpin, режимы и ручная сборка. |

[Описание в общем каталоге](../../README.md#example-embedding-manual_collection).
<!-- example-catalog-api:end -->

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_gc_collect` | function | [include/flashvm/gc.h:54](../../../include/flashvm/gc.h#L54) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:60](../../../include/flashvm/gc.h#L60) |
| `fvm_gc_unpin` | function | [include/flashvm/gc.h:70](../../../include/flashvm/gc.h#L70) |
<!-- source-links:end -->
