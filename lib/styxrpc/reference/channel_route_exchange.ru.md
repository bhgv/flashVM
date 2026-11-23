# styxrpc_channel_route_exchange

[English](channel_route_exchange.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/channel_route.h>
StyxRpcStatus styxrpc_channel_route_exchange(struct StyxClient *session, const char *module_bus,
    const StyxRpcChannelRoute *route, const StyxRpcSelection *selection, unsigned timeout_ms,
    void *scratch, size_t scratch_size, void *request, size_t request_capacity,
    void *reply, size_t reply_capacity, StyxRpcChannelResult *result);
```

Выполните одну синхронную отправку, приём или выбор по делегированному
маршруту `route`. Передайте точное подключение `session`, по которому
получили полномочие. Для PID передайте нулевой `module_bus`. Адаптер откроет
`.prog/<pid>/.unibus`. Для MODULE передайте относительный путь к `.unibus`
точного модуля. Адаптер отправит токен отдельно со смещения ноль, запишет
операцию и прочитает ответ со смещения ноль. CURRENT обслуживается кодеком
на той же шине, где получено значение.

Все ветки `selection` используют `route->channel.key`. Буферы в байтах не
пересекаются. `scratch` и `scratch_size` используются для сборки обёртки ALT.
В `request` и `request_capacity` помещается закодированный запрос, в
`reply` и `reply_capacity` полный кадр ответа. Типизированные данные
`result` заимствуют буфер до переиспользования. Удерживайте сессию,
полномочие и заимствованные данные до завершения операции.

`timeout_ms` от 1 до 60000 проверяется между синхронными транзакциями Styx.
Временный fid операции закрывается при любом исходе. `STYXRPC_OK` публикует
результат. `STYXRPC_MORE` означает истечение срока. `STYXRPC_REMOTE_ERROR`
означает подтверждённую ошибку канала. `STYXRPC_INVALID` возвращается при
неверном аргументе, ошибке передачи или авторизации, отозванном маршруте либо
неверном кадре. Операция могла завершиться до истечения срока. Автоповтор
запрещён. Подключите `StyxRpc::launch`.

[styxrpc_channel_exchange](channel_exchange.ru.md) · [Пример](../examples/disvm/routed_client.c) · [Исходник](../src/channel_exchange.c)
