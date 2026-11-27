# styxrpc_named_peer_host_scoped

[English](named_peer_host_scoped.md) · [API](../include/styxrpc/named_peer.h#L38) · [Исходник](../src/named_peer.c#L238) · [Руководство](module_refs.ru.md)

`StyxRpc::serve`

```c
StyxRpcNamedPeer *styxrpc_named_peer_host_scoped(struct RemotePhy *lane,
    StyxRpcNamedRegistry *registry, unsigned msize, unsigned timeout,
    unsigned tags, const void *scope);
```

## Контракт

Передайте `lane` и заимствованный `registry`, как при [создании хоста](named_peer_host.ru.md). `msize` задаётся в байтах, `timeout` в миллисекундах, `tags` ограничивает число ожидающих локальных запросов Styx. Передайте в `scope` 16 новых случайных байтов для этой пары либо NULL для отключения возможности ссылок на модули. Хост копирует байты до запуска потоков службы.

Вызов принимает владение `lane` при любом результате и может ждать подключения локального клиента. Он выдаёт собственный объект хоста либо NULL при неудаче. Сохраняйте реестр до [уничтожения хоста](named_peer_destroy.ru.md). Встречная сторона может [прочитать область](named_peer_read_ref_scope.ru.md) после подключения к потоку службы имён.
