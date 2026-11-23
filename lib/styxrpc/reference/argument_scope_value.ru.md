# styxrpc_argument_scope_value

[English](argument_scope_value.md) · [Объявление](../include/styxrpc/channel_arguments.h#L41) · [Реализация](../src/channel_arguments.c) · [Сценарий](channel_argument_scopes.ru.md)

Подключите `StyxRpc::launch`.

```c
#include <styxrpc/channel_arguments.h>
StyxRpcStatus styxrpc_argument_scope_value(StyxRpcArgumentScope *scope, StyxRpcChannelQueue *queue,
    void *bytes, size_t capacity, size_t *used);
```

Передайте область подготовки `scope` и собственную `queue`, задав точную форму одного элемента в скобках, память байтов, длины элементов, число слотов, размер слота и корректные курсоры. Идентификатор очереди — её адрес. Повторные ссылки получают тот же ключ текущей шины в пределах области. Допускается до восьми разных очередей. Задайте собственный буфер `bytes` ёмкостью `capacity` байт. `STYXRPC_OK` записывает размер в `used`. Неверный вход, исчерпанные ключи и нехватка места дают `STYXRPC_INVALID`. Хранилище очереди закрепляется за активным вызовом до отзыва.

[Проверка C](../tests/channel_arguments.c)
