# styxphywalkinit

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](styxphywalkinit.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/server/server.h>

char *styxphywalkinit(Styxserver *server,
                      Styxops *ops,
                      RemotePhy *phy,
                      int permissions,
                      int needfile,
                      unsigned queued_frames,
                      const WalkEntity *root);
```

## Библиотека

Styx::phy_server; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `server` | `Styxserver *server` | Экземпляр исходного сервера и состояние его dispatcher. |
| `ops` | `Styxops *ops` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `phy` | `RemotePhy *phy` | Capability физического носителя; передача владения зависит от конструктора/операции. |
| `permissions` | `int permissions` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `needfile` | `int needfile` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `queued_frames` | `unsigned queued_frames` | Скалярный операнд операции; диапазон протокола/алгоритма описан ниже. |
| `root` | `const WalkEntity *root` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |

## Описание

styxphywalkinit: реализация phy/traversal и нативное состояние.

### Сохранённый контракт implementation/header

EN/RU: Explicit S-record active-history profile around original Styx; PHY consumed always, root copied on success.
Both peers opt in; root describes the virtual exported endpoint of the actual prepared grant and must have nonzero domain/object.
Tag0 negotiates data purpose; malformed profiles close without raw fallback. Same owner/quota rules as styxphyinit.
Явный S-record history profile; PHY принят всегда, root — virtual exported endpoint actual grant с nonzero domain/object.
Обе стороны opt in; tag0 согласует purpose, malformed profiles закрываются без raw fallback.

## Возвращаемое значение

`char *`; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::phy_server; hosted native build

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: server/phy/traversal.c:70](../../server/phy/traversal.c#L70)
- [Public declaration](../../server/phy/server.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/disVM/native/README.md)
