# styxrpc_argument_scope_close

[English](argument_scope_close.md) · [Объявление](../include/styxrpc/channel_arguments.h#L52) · [Реализация](../src/channel_arguments.c) · [Сценарий](channel_argument_scopes.ru.md)

Подключите `StyxRpc::launch`.

```c
#include <styxrpc/channel_arguments.h>
void styxrpc_argument_scope_close(StyxRpcArgumentScope *scope);
```

Поглотите собственную `scope`. Для `NULL` действий нет. В активной области отзываются ключи и отменяются ожидающие встречные операции. Курсоры исходных очередей получают результаты завершённых операций. Отсоединённые fid отклоняют последующий ввод-вывод до clunk встречного сервера. После возврата локальный доступ к очередям снова доступен.

[Проверка C](../tests/channel_arguments.c)
