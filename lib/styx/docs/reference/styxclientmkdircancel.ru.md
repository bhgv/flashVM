# styxclientmkdircancel

[English](styxclientmkdircancel.md) · [API Styx](../index.ru.md) · [Обычный mkdir](styxclientmkdir.ru.md)

```c
#include <styx/client/client.h>
int styxclientmkdircancel(StyxFid *directory, const char *name, unsigned permissions,
    StyxCancel *cancel, StyxTransfer *result);
```

Хостовая сборка C99, цель `Styx::client`.

| Параметр | Значение |
| --- | --- |
| `directory` | Собственный неоткрытый fid родителя. Согласуйте использование и закрытие. |
| `name` | Заимствованный компонент короче `STYX_NAME_MAX`, кроме точки, двух точек и имён со слешем. |
| `permissions` | Запрошенные биты прав каталога в пределах восьмеричного `0777`. |
| `cancel` | Заимствованный необязательный одноразовый токен мультиплексного транспорта. `NULL` сохраняет обычный срок. |
| `result` | Обязательный выходной нативный результат. Число байт равно нулю. |

Функция отправляет один `Tcreate` с битом каталога и режимом чтения.
При `Rcreate` возвращает `1` и оставляет тот же fid открытым на новом каталоге.
Локальный отказ, `Rerror`, отмена или потеря ответа дают `0`. Отправленный
запрос без ответа выставляет `uncertain` и закрывает этому fid дальнейшие
операции открытия и обмена. Вызывающий сохраняет владение fid и освобождает его.

```c
StyxFid *parent = styxclientwalk(client, ".");
StyxTransfer result;
int made = styxclientmkdircancel(parent, "folder", 0777, token, &result);
styxclientclunklater(parent);
```

[Токен опроса](styxcancelcreatepoll.ru.md) · [Заголовок](../../client/client.h) · [Реализация](../../client/open.c)
