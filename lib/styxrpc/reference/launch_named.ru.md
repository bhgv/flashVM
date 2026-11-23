# styxrpc_launch_named

[English](launch_named.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/launch.h>
StyxRpcLaunchStatus styxrpc_launch_named(struct StyxClient *client, const char *name,
    const StyxRpcProgram *program, const void *arguments, size_t size,
    void *scratch, size_t capacity, struct StyxFid **request);
```

Используйте подключённый двусторонний клиент Styx. Передайте публичное имя с окончанием `.dis` длиной до 31 байта. Вызывающая сторона заранее публикует ненулевой `program->pid` и, если задан, отличный от него PID родителя в `.prog` встречного потока. `hidden_fds` скрывает наследуемые FD по номеру бита. `arguments` содержит `size` байт, включая NUL. Полная запись должна помещаться в отдельный scratch и в 64 байта провайдера. Инициализируйте `*request` как NULL. При PENDING вызывающий получает fid. Удерживайте встречное дерево до его закрытия. NOT_SENT и UNKNOWN не возвращают fid. При UNKNOWN программа могла стартовать, а отмену или результат подтвердить не удалось.

Hosted Linux64. `StyxRpc::launch`.

[Пример](../examples/disvm/named.c) · [Исходник](../src/launch.c)
