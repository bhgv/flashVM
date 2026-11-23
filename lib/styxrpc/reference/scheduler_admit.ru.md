# styxrpc_scheduler_admit

[English](scheduler_admit.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/channel_scheduler.h>
int styxrpc_scheduler_admit(void *context, StyxRpcChannelFid *fid);
```

Установите как колбэк `admit` открытого `fid` канала. В `context` передайте
настроенный планировщик. Fid вызывает колбэк один раз после полной приёмки и
проверки операции. Планировщик сверяет ключи и типы всех веток с очередями и
пытается выполнить готовую операцию.

Возвращает 1 при допуске, включая немедленное завершение, и 0 при заполнении
очереди операций, неизвестной ветке, несовпадении формы, повторной постановке
fid или отсутствии часов при WAIT. Планировщик удерживает fid и его буферы до
завершения или отмены. Функция выполняется без блокировки и ввода-вывода.
Владелец сериализует обращения к планировщику. Подключите `StyxRpc::rpc`.

[styxrpc_scheduler_cancel](scheduler_cancel.ru.md) · [Пример](../examples/disvm/owned_port.c) · [Исходник](../src/channel_scheduler.c)
