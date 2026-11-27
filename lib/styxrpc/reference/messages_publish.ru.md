# styxrpc_messages_publish

[English](messages_publish.md) · [messages.h](../include/styxrpc/messages.h) · [messages_enable](messages_enable.ru.md) · [UniBus](custom_messages.ru.md)

`StyxRpc::serve`

```c
StyxRpcStatus styxrpc_messages_publish(StyxRpcServer *server, const char *name,
    const StyxRpcMessageSpec *spec);
```

Поток владельца публикует до первого шага или во время работы сервера, вне
колбэков обмена. Сервер копирует имя `name` и реестр `spec`. Коллизия имени
или Qid, нехватка места либо ошибочный описатель дают `STYXRPC_INVALID`.

Имя — один компонент корня Styx. NULL выбирает `.unibus`. Вызывает поток владельца.

Динамическое дерево резервирует опубликованные имена и диапазон Qid обмена
(начиная с UINT64_MAX − 1 по убыванию). Дерево слота модуля резервирует точное
имя от обычных файлов и именованных каналов до [отзыва](messages_withdraw.ru.md).
Состояние колбэков действует до отзыва или уничтожения сервера.
