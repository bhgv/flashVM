# Native object methods / Нативные методы объектов

**EN:** This C89 example registers `add`, creates an object with `base = 35`,
then calls `fvm_object_call(context, object, "add", 1, ...)`. The callback
receives that object as `self`, reads its `base` property and returns 42.

**RU:** Пример на C89 регистрирует `add`, создаёт объект со свойством
`base = 35` и вызывает `fvm_object_call`. Обработчик получает объект в
`self`, читает `base` и возвращает 42.

Build / Сборка: `cmake -S . -B build && cmake --build build`.
Run / Запуск: `build/flashvm_example_object_methods`.

**Limitations / Ограничения:** Methods are currently resolved by name in the
native callback registry, not through function-valued object properties.
The `.as` file illustrates intended syntax, not a currently working
ActionScript integration test. `CallMethod` AVM1 supports this native bridge.

Методы пока ищутся по имени в реестре C-функций, а не как функции-свойства
объекта. Файл `.as` иллюстрирует планируемый синтаксис, но ещё не является
проходящим интеграционным тестом ActionScript.

<!-- example-catalog-api:start -->
## Назначение и особенности

Регистрирует native-функцию, связывает её с объектом и вызывает как метод. Позволяет увидеть, как receiver/self и аргументы отличаются от обычного глобального вызова.

**Текущая CMake-цель:** `flashvm_examples_embedding_object_methods_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_object_call`](../../../include/flashvm/value.h) | [AVM1_FUNCTIONS.md](../../../doc/AVM1_FUNCTIONS.md) — Метод, receiver и аргументы. |
| [`fvm_object_create`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_object_get`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_object_set`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |
| [`fvm_register_function`](../../../include/flashvm/value.h) | [API_INDEX.md#native-calls-and-metatables](../../../doc/API_INDEX.md#native-calls-and-metatables) — Регистрация именованного native callback. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_value_number`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../../README.md#example-embedding-object_methods).
<!-- example-catalog-api:end -->
