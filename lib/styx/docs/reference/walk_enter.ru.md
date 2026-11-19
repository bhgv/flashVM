# walk_enter

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](walk_enter.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/walk/walk.h>

int walk_enter(WalkScope *scope,
               const WalkEntity *entity,
               unsigned long high,
               unsigned long low,
               unsigned operation,
               const char *remaining,
               const char *detail);
```

## Библиотека

Styx::walk; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `scope` | `WalkScope *scope` | Локальный guard активного обхода запроса; успешный enter требует соответствующего leave. |
| `entity` | `const WalkEntity *entity` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `high` | `unsigned long high` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `low` | `unsigned long low` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `operation` | `unsigned operation` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `remaining` | `const char *remaining` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `detail` | `const char *detail` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

walk_enter: реализация walk/walk и нативное состояние.

### Сохранённый контракт implementation/header

EN/RU: Push exact entity/node/operation/remaining/detail state; return WALK_* and enter only on OK.
Borrow strings/entity through leave; node is an unsigned32-limb object identifier, not a bind occurrence.
Top-level invocation owns a new search; parallel native threads have independent histories.
Push exact entity/node/operation/remaining/detail; WALK_*, scope возникает лишь при OK.
Strings/entity borrowed до leave; node — unsigned32 limbs, не occurrence bind; parallel histories независимы.

## Возвращаемое значение

`int`; прямые константы результата тела: WALK_CYCLE, WALK_INVALID, WALK_LIMIT, WALK_NOMEM, WALK_OK. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::walk; hosted native build

## Пример

Фрагмент настоящего вызова; инициализация переменных и cleanup находятся в окружающем исходнике.

```c
result = walk_enter(&entry, &entity, high, low, operation, name, mode);
```

[Complete calling context](../../walk/wire.c#L237)

## См. также

- [Implementation: walk/walk.c:206](../../walk/walk.c#L206)
- [Public declaration](../../walk/walk.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
