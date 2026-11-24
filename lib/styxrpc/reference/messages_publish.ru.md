# styxrpc_messages_publish

[English](messages_publish.md) · [messages.h](../include/styxrpc/messages.h) · [messages_enable](messages_enable.ru.md) · [UniBus](custom_messages.ru.md)

`StyxRpc::serve`

```c
StyxRpcStatus styxrpc_messages_publish(StyxRpcServer *server, const char *name,
    const StyxRpcMessageSpec *spec);
```

До первого шага сервер `server` копирует имя `name` и реестр `spec`. Коллизия имени или Qid, нехватка места либо ошибочный описатель дают `STYXRPC_INVALID`.

Имя — один компонент корня Styx. NULL выбирает `.unibus`. Вызывает поток владельца.

Динамическое дерево должно резервировать имена опубликованных корней, их Qid
(начиная с UINT64_MAX − 1 по убыванию) и позиции в корне до уничтожения сервера.
