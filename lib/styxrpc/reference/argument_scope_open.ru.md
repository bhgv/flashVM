# styxrpc_argument_scope_open

[English](argument_scope_open.md) · [Объявление](../include/styxrpc/channel_arguments.h#L36) · [Реализация](../src/channel_arguments.c) · [Сценарий](channel_argument_scopes.ru.md)

Подключите `StyxRpc::launch`.

```c
#include <styxrpc/channel_arguments.h>
StyxRpcArgumentScope *styxrpc_argument_scope_open(StyxRpcArgumentBus *bus, uint32_t pid);
```

Зарезервируйте область подготовки на `bus` для её точного ненулевого `pid`. Функция возвращает собственную область или `NULL` при неверной шине или PID, занятой шине либо нехватке памяти. Ключи подготовки недоступны встречному дереву. Передайте область в [invoke](argument_scope_invoke.ru.md) или [close](argument_scope_close.ru.md).

[Проверка C](../tests/channel_arguments.c)
