# styxrpc_server_observe_attach

[English](server_observe_attach.md) · [Заголовок](../include/styxrpc/serve.h#L54) · [Реализация](../src/serve_attach.inc) · [Шаг сервера](server_step_for.ru.md)

```c
#include <styxrpc/serve.h>

void styxrpc_server_observe_attach(StyxRpcServer *server,
    StyxRpcAttachNotice notice, void *state);
```

Установить наблюдатель в потоке владельца между шагами сервера.
`server` обозначает живой диспетчер. При NULL ничего не происходит.
`notice` получает `state` и заимствованное имя attach с завершающим NUL
при обработке запроса до ответа. Наблюдатель не возвращает значение и не
выдаёт права. Сохраняйте `state` до замены или снятия регистрации.
NULL в `notice` снимает её. Скопируйте имя, если оно нужно после вызова.
Наблюдение синхронно и не допускает повторного входа в сервер.
Установка не выполняет I/O. Линковать с `StyxRpc::serve`.

[Подключение службы имён](named_peer_host.ru.md) использует это имя для
согласования возможности на уже выданном соединении.
