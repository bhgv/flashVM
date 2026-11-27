# styxrpc_channel_call_reply

[English](channel_call_reply.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/channel_call.h>
StyxRpcStatus styxrpc_channel_call_reply(const StyxRpcSchema *schema, const StyxRpcChannelCall *call,
    const void *result, size_t result_size, const void *updates, size_t update_size,
    void *bytes, size_t capacity, size_t *used);
```

Заимствует исходный допущенный `call` и точную `schema`. Передайте полную запись результата, полную запись обновлений с тегом 19 и их длины в байтах. Пустой список обновлений тоже передаётся записью с тегом 19. Входы не пересекаются с буфером `bytes`. `capacity` задаёт доступный размер, полный ответ ограничен 65 536 байтами. При `STYXRPC_OK` в `used` записана длина ответа. `STYXRPC_INVALID` сообщает о неверной схеме, идентификаторе ref, типе канала, некорректных изменениях или нехватке места. При отказе переданный `used` обнуляется. Функция проверяет и кодирует локально без обмена с другой стороной.

`StyxRpc::rpc`.

[Пример](../examples/disvm/channel_device.c) · [Исходник](../src/channel_call.c)
