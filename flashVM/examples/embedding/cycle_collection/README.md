# Cycle Collection

A tracing collector can reclaim mutually referencing objects once no root reaches the cycle.

**Русский:** Сборка циклических ссылок, недостижимых из корней.

<!-- example-catalog-api:start -->
## Назначение и особенности

Концептуальная заметка о сборке взаимно ссылающихся объектов после исчезновения всех корней. Подходит для объяснения отличия трассирующего GC от подсчёта ссылок; самостоятельного исходника здесь нет.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_gc_collect`](../../../include/flashvm/gc.h#L54) | [GARBAGE_COLLECTION.md#api](../../../doc/GARBAGE_COLLECTION.md#api) — Корни, pin/unpin, режимы и ручная сборка. |
| [`fvm_object_create`](../../../include/flashvm/value.h#L84) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_object_set`](../../../include/flashvm/value.h#L106) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../../README.md#example-embedding-cycle_collection).
<!-- example-catalog-api:end -->

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_gc_collect` | function | [include/flashvm/gc.h:54](../../../include/flashvm/gc.h#L54) |
| `fvm_object_create` | function | [include/flashvm/value.h:84](../../../include/flashvm/value.h#L84) |
| `fvm_object_set` | function | [include/flashvm/value.h:106](../../../include/flashvm/value.h#L106) |
<!-- source-links:end -->
