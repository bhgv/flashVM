# Internal containers and metatable migration

## English

FlashVM's internal `std::vector` and `std::map` dependencies have been replaced
by C++98 implementations in `src/details/vector.hpp` and
`src/details/hash_table.hpp`. The vector is a contiguous, dynamically resized
array. The hash table uses buckets for average constant-time lookup and a linked
insertion-order list for deterministic iteration. Both are internal, not ABI.

The object currently has **two distinct stores**: a named hash table and a
numeric hash table. A unified adaptive Lua-style dense-array + sparse-hash
container and queue operations are **not implemented yet**. `std::string`
remains an internal dependency; no STL types appear in the public C ABI.

### Metatable semantics currently implemented

- `fvm_table_set_metatable(ctx, object, meta)` attaches a *reference* to a
  shared table; `NULL` removes it. `fvm_table_get_metatable` returns that same
  object pointer, not a copy. Both operations require objects owned by `ctx`.
- `__index` may hold an object/table: when a named property is absent from
  the receiver and its AVM1 prototype chain, lookup continues in that table.
- `__newindex` may hold an object/table: writes of *absent* named properties
  are redirected to that table. Existing properties are assigned directly.
- Cycles in metatable attachment are rejected; lookup also has a depth limit.
- Numeric indexing still uses the legacy direct metatable fallback, rather
  than `__index` semantics. This is a transitional compatibility limitation.
- Function-valued metamethods, primitive-type metatables, user data,
  callable table values, lexical environment replacement, and the complete
  Lua metamethod set are **not yet implemented**. Lua semantics and Flash AVM1
  semantics are not identical and must be reconciled explicitly.

### C API example

See `tests/metamethod_tables.c` for a compilable C89 example that installs a
shared metatable, performs `__index` lookup, redirects `__newindex`, then
removes the metatable.

## Русский

Внутренние зависимости от `std::vector` и `std::map` заменены собственными
контейнерами C++98 в `src/details/`. Вектор — расширяемый непрерывный массив.
Хэш-таблица содержит корзины для быстрого поиска и связанный список для
детерминированного перечисления. Публичный C ABI не содержит типов STL.

**Пока это переходная реализация:** именованные и числовые поля хранятся в
двух отдельных хэш-таблицах. Общая гибридная структура «плотный массив +
разреженный хэш + очередь» ещё не готова. Внутри остаётся `std::string`.

### Метатаблицы

- `fvm_table_set_metatable` назначает ссылку на обычную таблицу; `NULL`
  удаляет метатаблицу. `fvm_table_get_metatable` возвращает указатель на
  оригинальную таблицу, а не её копию.
- `__index` со значением-таблицей используется при отсутствии именованного
  свойства после поиска по цепочке прототипов AVM1.
- `__newindex` со значением-таблицей перенаправляет запись нового свойства.
  Существующее собственное свойство изменяется напрямую.
- Циклические ссылки метатаблиц запрещены, глубина поиска ограничена.
- Числовые индексы пока используют прежний прямой поиск в метатаблице.
- Функциональные метаметоды, метатаблицы примитивных значений, userdata,
  скрытые пространства имён и полная семантика Lua **ещё не реализованы**.

Компилируемый пример C89 с `__index` и `__newindex`:
`tests/metamethod_tables.c`.
