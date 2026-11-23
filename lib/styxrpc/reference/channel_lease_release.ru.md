# styxrpc_channel_lease_release

[English](channel_lease_release.md) · [Объявление](../include/styxrpc/channel_client.h#L37) · [Реализация](../src/channel_client.c) · [Описание аренды](channel_leases.ru.md)

Подключите `StyxRpc::launch`.

```c
#include <styxrpc/channel_client.h>
void styxrpc_channel_lease_release(StyxRpcChannelLease *lease, int deferred);
```

Поглотите одну собственную ссылку `lease`. Допускается `NULL`. Последняя ссылка закрывает fid функции и fid модуля, отзывая удалённый токен. Ноль в `deferred` выбирает синхронный clunk Styx. Ненулевое значение передаёт закрытие отложенной очистке Styx, например из финализатора владельца.

Освобождайте каждую ссылку однократно и согласуйте вызов с операциями аренды. Для немедленного удалённого отзыва сохраняйте сессию активной до синхронного clunk. Завершение сессии также отзывает действующие полномочия.

[Исполняемая проверка](../examples/disvm/routed_client.c)
