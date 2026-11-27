# styxclientfidcancellable

[English](styxclientfidcancellable.md) · [API Styx](../index.ru.md) · [Транспорт клиента](styxclientcancellable.ru.md)

```c
#include <styx/client/client.h>
int styxclientfidcancellable(StyxFid *fid);
```

Хостовая сборка C99, цель `Styx::client`.

Передайте заимствованный `fid`, чей владелец исключает одновременный clunk.
Функция возвращает `1` для fid на мультиплексном транспорте и `0` для `NULL`
или последовательного транспорта. Поддержка отмены фиксируется при создании
транспорта. Сетевого обмена и опроса сервера нет. Владение fid остаётся у
вызывающего.

Для удерживаемого `file` выберите доступный режим ожидания метаданных.
Вызывающему принадлежат `token` и выходная структура `meta`.

```c
StyxTransfer reply;
int read = styxclientfidcancellable(file) ?
    styxclientstatcancel(file, &meta, token, &reply) :
    styxclientstat(file, &meta);
```

[Заголовок](../../client/client.h) · [Реализация](../../client/fid.c)
