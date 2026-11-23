# styxrpc_channel_handlers_step

[English](channel_handlers_step.md) · [Объявление](../include/styxrpc/channel_handlers.h) · [Реализация](../src/channel_handlers.c) · [Сценарий](channel_handler_modules.ru.md) · [Проверка C](../tests/channel_handlers.c)

Подключите `<styxrpc/channel_handlers.h>` и линкуйте хостовую цель C99 `StyxRpc::serve` с поддержкой потоков POSIX.

```c
int styxrpc_channel_handlers_step(StyxRpcChannelHandlers *module);
```

Ждёт одну работу из очереди `module` и вызывает колбэк на текущем потоке. Используйте одного владельца без рекурсивных вызовов этой функции. Прямой диспетчер сервера работает отдельно. Во время колбэка диспетчер может подтверждать записи, читать готовые ответы и закрывать FID.

Возвращает 1 после обработки одной работы. Возвращает 0 для NULL либо после остановки модуля. Колбэк может ждать встречную операцию. Его аргументы заимствуют память проверенного запроса до возврата. Результат проверяется и публикуется после возврата колбэка.

[`create`](channel_handlers_create.ru.md) · [`stop`](channel_handlers_stop.ru.md) · [`destroy`](channel_handlers_destroy.ru.md)
