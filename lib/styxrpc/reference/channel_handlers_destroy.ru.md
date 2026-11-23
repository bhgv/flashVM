# styxrpc_channel_handlers_destroy

[English](channel_handlers_destroy.md) · [Объявление](../include/styxrpc/channel_handlers.h) · [Реализация](../src/channel_handlers.c) · [Сценарий](channel_handler_modules.ru.md) · [Проверка C](../tests/channel_handlers.c)

Подключите `<styxrpc/channel_handlers.h>` и линкуйте хостовую цель C99 `StyxRpc::serve` с поддержкой потоков POSIX.

```c
void styxrpc_channel_handlers_destroy(StyxRpcChannelHandlers *module);
```

Освобождает собственный `module` и его ресурсы синхронизации. Результат void. Допускается NULL. Сначала остановите модуль и дождитесь завершения владельца и сервера Styx, включая закрытие всех открытых FID. Хранилище описателей и контексты колбэков принадлежат приложению.

[`create`](channel_handlers_create.ru.md) · [`step`](channel_handlers_step.ru.md) · [`stop`](channel_handlers_stop.ru.md)
