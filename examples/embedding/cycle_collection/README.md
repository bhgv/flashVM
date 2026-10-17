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
| [`fvm_gc_collect`](../../../include/flashvm/gc.h) | [GARBAGE_COLLECTION.md#api](../../../doc/GARBAGE_COLLECTION.md#api) — Корни, pin/unpin, режимы и ручная сборка. |
| [`fvm_object_create`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_object_set`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../../README.md#example-embedding-cycle_collection).
<!-- example-catalog-api:end -->
