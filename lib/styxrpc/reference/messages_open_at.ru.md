# styxrpc_messages_open_at

[English](messages_open_at.md) · [messages.h](../include/styxrpc/messages.h) · [messages_open](messages_open.ru.md) · [UniBus](custom_messages.ru.md)

`StyxRpc::launch`

```c
StyxRpcStatus styxrpc_messages_open_at(struct StyxClient *session, const char *name,
    const StyxRpcMessageSpec *spec, StyxRpcMessagePort **out);
```

На подключённом клиенте `session` открывает указанный корень на чтение и запись. Копирует реестр `spec` и при `STYXRPC_OK` записывает принадлежащий клиенту порт в `out`. При `STYXRPC_INVALID` значение `out` сохраняется.

Имя — один компонент корня Styx. NULL выбирает `.unibus`. Вызывает поток владельца.
