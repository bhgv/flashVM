# styxrpc_scheduler_cancel

[English](scheduler_cancel.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/channel_scheduler.h>
void styxrpc_scheduler_cancel(void *context, StyxRpcChannelFid *fid);
```

Установите как колбэк `cancel` и передайте планировщик как `context`.
При закрытии ожидающего `fid` колбэк удаляет именно этот указатель из очереди
перед освобождением буферов. Если fid уже завершён или отсутствует, очередь
сохраняется. Уже выполненный обмен с буфером сохраняет результат.

Колбэк выполняется без ввода-вывода и не возвращает значение. Его вызывает
владелец планировщика. Подключите `StyxRpc::rpc`.

[styxrpc_channel_fid_close](channel_fid_close.ru.md) · [Пример](../tests/scheduler.c) · [Исходник](../src/channel_scheduler.c)
