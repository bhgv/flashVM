# styxwritereply

[English](styxwritereply.md) · [Оглавление API](../index.ru.md)

Завершить одну отложенную запись в потоке диспетчера.

```c
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>

int styxwritereply(StyxWrite *request, ulong count, char *error);
```

Функция доступна в нативных целях `Styx::server` и `Styx::phy_server`.
Заимствуйте живой `request` из [styxdeferwrite](styxdeferwrite.ru.md)
после возврата колбэка записи. При NULL в `error` подтвердите
префикс длиной `count` байтов, до длины исходной записи. Для `Rerror`
передайте `count = 0` и строку ошибки, действительную до возврата.

Функция возвращает 1, когда снимает запрос и пытается отправить ответ.
Она возвращает 0 до возврата колбэка, после отмены или завершения
и при неверном аргументе. Неверная длина оставляет живой запрос для
исправления. При снятии вызывается переданный при допуске колбэк
освобождения, который может закрыть ссылку вызывающего. В остальных
случаях вызывающий освобождает ссылку через
[styxwritefree](styxwritefree.ru.md).

[Тест диспетчера](../../vendor/inferno/tools/libstyx/tests/deferred-read.c)
вызывает `styxwritereply(request, 2, NULL)` для записи трёх байтов и
проверяет короткий счётчик `Rwrite`.

[Объявление](../../vendor/inferno/include/readable_parts/styxserver-h/part-002.inc#L194) ·
[Реализация](../../vendor/inferno/tools/libstyx/read.c#L182)
