# Lua-style native callback / Lua-подобный нативный callback

Build this C89 example through CMake or `python3 scripts/check.py`.
It registers `add`, supplies two numbers, and prints `add(20, 22) = 42`.
The same registration is callable by AVM1 ActionCallFunction; the bytecode
integration is covered by `tests/auxlib.c`.

Сборка: CMake или `python3 scripts/check.py`. Пример регистрирует `add`,
передаёт два числа и выводит `add(20, 22) = 42`. Ту же функцию можно вызвать
из AVM1 через ActionCallFunction; это проверено в `tests/auxlib.c`.

See / см. `doc/LUA_STYLE_API.md` and `doc/LUA_STYLE_API.ru.md`.

<!-- example-catalog-api:start -->
## Назначение и особенности

Регистрирует add через Lua-подобный стековый API, читает числовые аргументы и помещает сумму. Показывает компактный способ писать native-функции без совместимости с Lua ABI.

**Текущая CMake-цель:** `flashvm_examples_extending_lua_style_main`.


## Демонстрируемые функции C API

Имена ведут к публичным декларациям. Во второй колонке указан файл руководства
и раздел с контрактом; общие функции setup/cleanup включены для воспроизводимости.

| Функция | Где читать подробнее |
| --- | --- |
| [`fvm_call`](../../../include/flashvm/value.h) | [API_INDEX.md#native-calls-and-metatables](../../../doc/API_INDEX.md#native-calls-and-metatables) — Вызов именованной функции. |
| [`fvm_config_init`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_create`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_context_destroy`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_create`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h) | [API_INDEX.md#runtime-and-context](../../../doc/API_INDEX.md#runtime-and-context) — Жизненный цикл runtime/context; точные сигнатуры и ownership — в заголовке. |
| [`fvm_stack_checknumber`](../../../include/flashvm/auxlib.h) | [LUA_STYLE_API.md](../../../doc/LUA_STYLE_API.md) — Проверка аргументов и Lua-подобная регистрация. |
| [`fvm_stack_pushnumber`](../../../include/flashvm/auxlib.h) | [LUA_STYLE_API.md](../../../doc/LUA_STYLE_API.md) — Проверка аргументов и Lua-подобная регистрация. |
| [`fvm_stack_register_cfunction`](../../../include/flashvm/auxlib.h) | [LUA_STYLE_API.md](../../../doc/LUA_STYLE_API.md) — Проверка аргументов и Lua-подобная регистрация. |
| [`fvm_value_number`](../../../include/flashvm/value.h) | [API_INDEX.md#values-and-borrowed-views](../../../doc/API_INDEX.md#values-and-borrowed-views) — Типы значений, borrowed-данные и преобразования. |

[Описание в общем каталоге](../../README.md#example-extending-lua_style).
<!-- example-catalog-api:end -->
