# physelect

Copyright (c) 2026 bhgv — reference annotations; original component licenses remain applicable.

[English](physelect.md) · [Index](../index.ru.md)

## Синтаксис

```c
#include <styx/phy/phy.h>

RemotePhy *physelect(const PhyDriver *drivers, size_t count, const char *name, const void *parameters, size_t size);
```

## Библиотека

Styx::carrier; hosted native build

## Параметры

| Параметр | Объявление | Назначение и ограничения |
| --- | --- | --- |
| `drivers` | `const PhyDriver *drivers` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `count` | `size_t count` | Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием. |
| `name` | `const char *name` | Имя компонента либо схемы операции; функция проверяет свои ограничения кодирования/длины. |
| `parameters` | `const void *parameters` | Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации. |
| `size` | `size_t size` | Запрошенная длина в байтах; соответствующий буфер должен её покрывать. |

## Описание

physelect: реализация remote_phy/select и нативное состояние.

### Сохранённый контракт implementation/header

EN/RU: Select exactly one matching driver from borrowed table; NULL absent/ambiguous/invalid.
Driver-specific native parameters borrowed until its open returns; no fallback to another carrier.
Выбрать один driver из borrowed table; NULL absent/ambiguous/invalid, без fallback.

## Возвращаемое значение

`RemotePhy *`; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.

## Ошибки и владение

Соблюдать точный контракт выше: overlap input/output, capacity, отмену и правила освобождения. Протокольное подтверждение, результат native queue и завершённый удалённый эффект — разные результаты. Уведомление не даёт прав, неопределённая запись не повторяется автоматически.

## Доступность

Styx::carrier; hosted native build

## Пример

Использовать полные примеры слоя; точная реализация этого низкоуровневого входа приведена по ссылке ниже.

[Examples](../../examples/README.ru.md)

## См. также

- [Implementation: remote_phy/select.c:30](../../remote_phy/select.c#L30)
- [Public declaration](../../remote_phy/phy.h)
- [API index](../index.ru.md)
- [Protocol and lifetime](../protocol.ru.md)
- [Main-project help](https://github.com/bhgv/flashVM/blob/master/remote_phy/README.md)
