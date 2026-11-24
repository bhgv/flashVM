# styxrpc_control_create

[English](control_create.md) · [API](../include/styxrpc/control.h) · [Source](../src/control.c) · [Guide](../MODULES.ru.md)

`StyxRpc::serve`

```c
StyxRpcStatus styxrpc_control_create(const StyxRpcControlSpec *spec, StyxRpcControl **out);
```

## Контракт

Задайте положительный лимит байтов и три колбэка. Загрузка получает полное содержимое, управление получает разобранную команду, состояние записывает до 4096 байтов в выданный буфер и задаёт фактическое число. Колбэк возвращает 1 при успехе. SDK копирует описатель и заимствует контекст. Успех даёт `STYXRPC_OK` и новый описатель. При `STYXRPC_INVALID` значение `out` сохраняется. Владелец сериализует колбэки и сохраняет их контекст до закрытия сервера.
