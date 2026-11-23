# styxrpc_scheduler_init

[English](scheduler_init.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/channel_scheduler.h>
int styxrpc_scheduler_init(StyxRpcChannelScheduler *scheduler, StyxRpcChannelQueue *queues,
    size_t count, uint64_t (*clock_ms)(void *), void *context);
```

Инициализируйте планировщик с `count` очередями, от 1 до
`STYXRPC_CHANNEL_SLOTS`. Каждая свежая очередь имеет отдельный положительный
ключ и точную форму элемента в скобках. Обнулите head и count. Нулевая
`capacity` задаёт синхронный канал без буфера. При положительной ёмкости
выделите `bytes` размером `capacity * stride` байт и `sizes` с длиной для
каждой ячейки. `stride` вмещает полный отправляемый элемент и ограничен
`STYXRPC_MANAGED_LIMIT`. Сохраняйте записи и непересекающиеся буферы весь
срок жизни планировщика.

Для WAIT передайте монотонные часы `clock_ms` с единицей в миллисекундах и
их заимствованный `context`. Если WAIT не используется, часы могут быть
`NULL`. Планировщик и fid принадлежат одному сериализованному владельцу
и одной области полномочий. Разные маршруты или аренды требуют отдельных
экземпляров. Во всех fid одного экземпляра ключ должен обозначать тот же
канал весь срок жизни ожидающих элементов.

Возвращает 1 при успешной регистрации и 0 при ошибке параметров. Выполняется
без ввода-вывода и выделения памяти. Инициализируйте свежий планировщик или
сначала закройте ожидающие fid. Подключите `StyxRpc::rpc`.

[styxrpc_scheduler_admit](scheduler_admit.ru.md) · [Пример](../tests/scheduler.c) · [Исходник](../src/channel_scheduler.c)
