# styxrpc_messages_abandon

[English](messages_abandon.md) · [Заголовок](../include/styxrpc/messages.h) · [Реализация](../src/message_client.c) · [Закрытие](messages_close.ru.md)

```c
#include <styxrpc/messages.h>
void styxrpc_messages_abandon(StyxRpcMessagePort *port);
```

Освободить свободный клиентский порт в потоке владельца и отложить закрытие
fid до операции подключённой сессии. Используйте при сборке мусора, если
освобождение объекта исключает транспортный I/O. Сессия должна пережить вызов.
Очередь и зарегистрированные имена освобождаются сразу. Допустим null-порт.
Вызывайте вне pump и колбэков и затем отбросьте указатель. Для синхронного
закрытия есть [styxrpc_messages_close](messages_close.ru.md).
