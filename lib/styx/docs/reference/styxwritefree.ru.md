# styxwritefree

[English](styxwritefree.md) · [Оглавление API](../index.ru.md)

Освободить ссылку вызывающего на отложенную запись в потоке сервера.

```c
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>

void styxwritefree(StyxWrite *request);
```

Функция доступна в `Styx::server` и `Styx::phy_server`. Передайте свою
ссылку `request` из [styxdeferwrite](styxdeferwrite.ru.md) или NULL.
Диспетчер удерживает ссылку, пока запрос активен. Его можно завершить
через [styxwritereply](styxwritereply.ru.md) или штатным снятием по
протоколу. Вызывающий может освободить ссылку из колбэка снятия после
отсоединения запроса.

```c
if (styxwritereply(request, accepted_bytes, NULL)) {
    styxwritefree(request);
}
```

В этом примере колбэк снятия оставляет ссылку вызывающему коду. Для
исправления неверного счётчика удерживайте ссылку до повторного вызова.
[Тест диспетчера](../../vendor/inferno/tools/libstyx/tests/deferred-read.c)

[Объявление](../../vendor/inferno/include/readable_parts/styxserver-h/part-002.inc#L199) ·
[Реализация](../../vendor/inferno/tools/libstyx/read.c#L189)
