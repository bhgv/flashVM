# styxrpc_program_server_revoke

[English](program_server_revoke.md) · [libstyxrpc](../README.ru.md) · [Объявление](../include/styxrpc/program.h#L141) · [Реализация](../src/serve_grants.inc#L21)

## Синтаксис

```c
#include <styxrpc/program.h>
int styxrpc_program_server_revoke(StyxRpcServer *server, uint32_t pid);
```

Скрывает встречный `.unibus` точного процесса `pid` на [сервере программ](program_server_create.ru.md). Для временной выдачи также скрывается её каталог. Операции в ранее открытых fid после отзыва завершаются ошибкой. Clunk по-прежнему освобождает сетевой fid. Освободившиеся слоты готовы к выдаче нового PID.

Возвращает 1 при отзыве и 0 при неверных параметрах либо отсутствии ресурса. До успешного возврата открытые ресурсы закрываются ровно один раз в отзывающем потоке. Вызов может ждать текущий запрос. Вызывайте вне колбэков сервера. Храните `server` и данные колбэков до завершения. Освобождайте ресурс после отзыва. Линкуйте `StyxRpc::serve`.

[Проверка на C](../tests/program_server.c) оставляет старый fid открытым во время отзыва и отдельно проверяет новую выдачу. [Выдать новый PID](program_server_grant.ru.md)
