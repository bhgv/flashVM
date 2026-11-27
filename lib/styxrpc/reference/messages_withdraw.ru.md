# styxrpc_messages_withdraw

[English](messages_withdraw.md) · [messages.h](../include/styxrpc/messages.h#L72) · [Публикация](messages_publish.ru.md)

`StyxRpc::serve`

```c
StyxRpcStatus styxrpc_messages_withdraw(StyxRpcServer *server, const char *name);
```

Вызывайте в потоке владельца вне колбэков обмена. Передайте действующий
`server` и точный компонент корня в `name`. NULL выбирает `.unibus`.
`STYXRPC_OK` означает, что корень снят, открытые и обойдённые fid отозваны,
а все ожидавшие хуки закрытия выполнены. После возврата можно освободить
состояние колбэков отозванного корня. `STYXRPC_INVALID` означает, что сервер
или указанный корень отсутствует.

Для замены отзовите существующий корень и опубликуйте его под тем же именем.
Клиенты должны заново пройти путь и открыть новые fid. Сервер сохраняет старые
fid отдельно до их закрытия клиентом.
