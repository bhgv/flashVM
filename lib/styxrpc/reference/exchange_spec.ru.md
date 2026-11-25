# styxrpc_exchange_spec

[English](exchange_spec.md) · [Заголовок](../include/styxrpc/exchange.h) · [Реализация](../src/exchange.c) · [Руководство](exchange.ru.md)

```c
#include <styxrpc/exchange.h>
void styxrpc_exchange_spec(StyxRpcExchange *exchange, int hosted,
    StyxRpcMessageType types[STYXRPC_EXCHANGE_END - 1], StyxRpcMessageSpec *spec);
```

Заполняет массив types и spec вызывающего всеми смешанными схемами версии 1. Передайте
живую очередь и доступные для записи результаты. Клиент передаёт hosted равным нулю.
Сервер передаёт ненулевое значение и допускает один fid, при его закрытии входная
очередь очищается. Держите выходы до копирования в open или publish. Контексты колбэков
заимствуют exchange до закрытия. Используйте одну регистрацию на очередь.
