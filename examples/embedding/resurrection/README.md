# Resurrection

A __gc callback can resurrect an object by attaching it to a reachable table; the collector marks again after finalization.

**Русский:** Финализатор может вернуть объект в достижимый граф; выполняется повторная маркировка.

<!-- example-catalog-api:start -->
## Назначение и особенности

Описывает возвращение объекта в достижимый граф из __gc и повторную маркировку после финализации. Это концептуальная заметка о контракте финализатора, а не пример с отдельным main.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_gc_collect`](../../../include/flashvm/gc.h) | [GARBAGE_COLLECTION.md#api](../../../doc/GARBAGE_COLLECTION.md#api) — Корни, pin/unpin, режимы и ручная сборка. |
| [`fvm_object_set`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../../README.md#example-embedding-resurrection).
<!-- example-catalog-api:end -->
