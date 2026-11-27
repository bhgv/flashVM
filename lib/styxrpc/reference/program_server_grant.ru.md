# styxrpc_program_server_grant

[English](program_server_grant.md) · [libstyxrpc](../README.ru.md) · [Объявление](../include/styxrpc/program.h#L134) · [Реализация](../src/serve_grants.inc#L7)

## Синтаксис

```c
#include <styxrpc/program.h>
uint32_t styxrpc_program_server_grant(StyxRpcServer *server, const StyxRpcResource *resource);
```

Добавляет `.prog/<pid>/.pid` и `.prog/<pid>/.unibus` на работающем [сервере программ](program_server_create.ru.md). Сервер копирует `resource` с обязательными колбэками чтения и записи. Храните его контекст и колбэки до отзыва. Используйте возвращённый PID на том же подключении Styx.

Возвращает новый ненулевой PID из верхней половины 32-битного диапазона. При неверных параметрах или исчерпании слотов либо PID возвращает ноль. Нужны три слота. Вызывайте из хоста или диспетчера вне колбэков этого сервера. Возможна задержка до завершения текущего запроса. Сам вызов не выполняет ввод-вывод по транспорту. Сохраняйте `server` до возврата. Линкуйте `StyxRpc::serve`.

[Проверка на C](../tests/program_server.c) выдаёт ресурс после монтирования у партнёра. [Отозвать выдачу](program_server_revoke.ru.md) · [Заимствовать очередь](channel_group_reverse_borrow.ru.md)
