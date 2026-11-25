# styxrpc_exchange_create

[English](exchange_create.md) · [Заголовок](../include/styxrpc/exchange.h) · [Реализация](../src/exchange.c) · [Руководство](exchange.ru.md)

```c
#include <styxrpc/exchange.h>
StyxRpcExchange *styxrpc_exchange_create(void);
```

Создаёт пустую отключённую очередь и возвращает собственный указатель либо NULL при
нехватке памяти. Аргументов нет, I/O не выполняется. Передайте очередь в exchange_spec,
чтобы зарегистрировать колбэки. Объект должен жить дольше всех зарегистрированных
портов.
