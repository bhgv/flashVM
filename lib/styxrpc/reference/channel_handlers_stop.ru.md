# styxrpc_channel_handlers_stop

[English](channel_handlers_stop.md) · [Объявление](../include/styxrpc/channel_handlers.h) · [Реализация](../src/channel_handlers.c) · [Сценарий](channel_handler_modules.ru.md) · [Проверка C](../tests/channel_handlers.c)

Подключите `<styxrpc/channel_handlers.h>` и линкуйте хостовую цель C99 `StyxRpc::serve` с поддержкой потоков POSIX.

```c
void styxrpc_channel_handlers_stop(StyxRpcChannelHandlers *module);
```

Останавливает приём в `module`, снимает очередь и будит владельца. Результат void. Допускается NULL. Можно вызывать из диспетчера либо другого потока хоста. Повторная остановка безопасна.

Остановка возвращается сразу, активный колбэк может завершиться. Открытый FID активного вызова может прочитать результат, если сервер остаётся соединён. При чтении FID снятой работы возвращается ошибка. Перед освобождением состояния колбэка или встречного клиента дождитесь владельца.

[`create`](channel_handlers_create.ru.md) · [`step`](channel_handlers_step.ru.md) · [`destroy`](channel_handlers_destroy.ru.md)
