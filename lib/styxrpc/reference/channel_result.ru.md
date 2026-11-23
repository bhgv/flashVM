# styxrpc_channel_result

[English](channel_result.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/channel.h>
StyxRpcStatus styxrpc_channel_result(const StyxRpcChannelGrant *grants, size_t grant_count,
    const StyxRpcSelection *operation, const void *bytes, size_t size, StyxRpcChannelResult *result);
```

Декодирует полный кадр `bytes` длиной `size` по исходной операции `operation` и таблице `grants` из `grant_count` записей. При `STYXRPC_OK` поле `result->selected` содержит номер ветки от единицы или ноль при отсутствии готовой ветки. Приём `int` заполняет `result->integer` и в скалярном, и в смешанном типизированном ответе. Типизированный приём также возвращает полный элемент через заимствованные `result->value` и `result->size` до повторного использования буфера. У ответа на отправку или пустой выбор принятого элемента нет.

`STYXRPC_REMOTE_ERROR` означает отказ участника. `STYXRPC_INVALID` означает неверный ввод или ответ. Обе ошибки сохраняют значение `result` вызывающего.

C99. `StyxRpc::rpc`.

[Пример](../tests/channel_socket.c) · [Исходник](../src/channel.c)
