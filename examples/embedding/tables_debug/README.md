# Table inspection / Инспекция таблиц

**English:** Compile as C89 and link with `flashvm`. This example writes a named property and a numeric slot, then queries object and property counts. No ActionScript integration of metatables exists yet.

**Русский:** Компилируйте как C89 и линкуйте с `flashvm`. Пример записывает именованное свойство и числовой элемент, затем получает количество свойств и объектов. Интеграция метатаблиц с ActionScript пока отсутствует.

ActionScript 2.0 (conceptual, only named properties currently supported):

```actionscript
var obj = new Object();
obj.answer = 42;
trace(obj.answer);
```

<!-- example-catalog-api:start -->
## Назначение и особенности

Записывает именованное и индексное поле, читает число имён и количество объектов VM. Минимальный пример сочетания table API с диагностической инспекцией без пользовательских getter.

**Текущая CMake-цель:** `flashvm_examples_embedding_tables_debug_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_debug_object_count`](../../../include/flashvm/debug.h) | [DEBUG_API.md#fvm_debug_object_count](../../../doc/DEBUG_API.md#fvm_debug_object_count) — Параметры, статусы, время жизни и ограничения вызова. |
| [`fvm_object_create`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_object_set`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_table_named_count`](../../../include/flashvm/table.h) | [STANDARD_OBJECTS.md#object-and-array](../../../doc/STANDARD_OBJECTS.md#object-and-array) — Таблицы, индексные поля и объектные графы. |
| [`fvm_table_set_index`](../../../include/flashvm/table.h) | [STANDARD_OBJECTS.md#object-and-array](../../../doc/STANDARD_OBJECTS.md#object-and-array) — Таблицы, индексные поля и объектные графы. |
| [`fvm_value_number`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../../README.md#example-embedding-tables_debug).
<!-- example-catalog-api:end -->
