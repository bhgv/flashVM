# styxdeferwrite

[English](styxdeferwrite.md) · [Оглавление API](../index.ru.md)

Оставить текущий `Twrite` в ожидании, пока сервер обслуживает другие теги.

```c
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>

StyxWrite *styxdeferwrite(Styxserver *server, void *aux, void (*release)(void *));
```

Функция доступна в нативных целях `Styx::server` и `Styx::phy_server`.
Вызывайте её из текущего колбэка записи в потоке сервера. Объект
`server` заимствуется. При успехе запрос удерживает `aux` для
необязательного `release(aux)`. Сервер вызывает этот колбэк один раз
при снятии запроса перед очисткой fid. Колбэк должен завершаться без
ожидания и повторного входа в диспетчер.

Ненулевой результат владеет одной ссылкой на запрос. Скопируйте вход
до возврата из колбэка записи, обнулите подтверждённую длину и верните
NULL. Нативный производитель может разбудить диспетчер по завершении
работы. Вызовите [styxwritereply](styxwritereply.ru.md) и
[styxwritefree](styxwritefree.ru.md) в потоке диспетчера.

При NULL `aux` остаётся у вызывающего. Для допуска нужна непустая
запись на обычном fid без другого отложенного запроса данных.
Общий предел составляет 128 отложенных запросов на клиента.
Сброс сессии, разрыв и `Tflush` снимают запрос. Clunk fid снимает
запрос с ошибкой по правилам [отложенных чтений](styxdeferread.ru.md).

[Тест диспетчера](../../vendor/inferno/tools/libstyx/tests/deferred-read.c)
удерживает запись трёх байтов одновременно с чтением другого fid.
Совпавший `Tflush` снимает запись, после чего чтение завершается.
Команда проверки `ctest --test-dir build -R '^styx-original-deferred-read$'`.

[Объявление](../../vendor/inferno/include/readable_parts/styxserver-h/part-002.inc#L185) ·
[Реализация](../../vendor/inferno/tools/libstyx/read.c#L108)
