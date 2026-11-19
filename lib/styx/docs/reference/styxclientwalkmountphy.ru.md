# styxclientwalkmountphy

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxclientwalkmountphy.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/client/client.h>

StyxClient *styxclientwalkmountphy(RemotePhy *phy,
                                   unsigned msize,
                                   const char *user,
                                   const char *name,
                                   unsigned timeout,
                                   unsigned limit,
                                   unsigned long tag);
```

## Библиотека

Styx::client; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `phy` | `RemotePhy *phy` | Capability физического носителя; передача владения зависит от конструктора/операции. |
| `msize` | `unsigned msize` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `user` | `const char *user` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `name` | `const char *name` | Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины. |
| `timeout` | `unsigned timeout` | Бюджет нативного ожидания в миллисекундах; поведение нуля и верхнего предела задано контрактом. |
| `limit` | `unsigned limit` | Явная локальная квота; диапазон и единицы wire/body указаны в описании. |
| `tag` | `unsigned long tag` | ID этого направления/пространства; внешний0 зарезервирован для служебного трафика. |

## Описание

styxclientwalkmountphy: нативные client-операции traversal и время жизни.

### Сохранённый контракт implementation/header

EN/RU: Explicit negotiated S-record history profile around unchanged Styx frames; consumes PHY always.
Both endpoints must enable the profile. tag is a nonzero direction-local assignment in tag0 controls.
No raw fallback/dial/authentication; active history must have exportable VM identities.
Явный S-record history profile, PHY принят всегда; обе стороны включают profile, tag согласуется через tag0.

## Возвращаемое значение

`StyxClient *`; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::client; hosted native build

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: client/traversal.c:35](../../client/traversal.c#L35)
- [Public declaration](../../client/client.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/client/README.md)
