# styxrpc_value_get_integer

[English](value_get_integer.md) · [Заголовок](../include/styxrpc/managed.h) · [Исходник](../src/managed.c)

`StyxRpcStatus styxrpc_value_get_integer(unsigned long tag, const void *bytes, size_t size, int64_t *value);`

Подключите `styxrpc/managed.h` и переносимую цель `StyxRpc::rpc`.
Передайте `STYXRPC_VALUE_INT`, `STYXRPC_VALUE_BIG` или `STYXRPC_VALUE_BYTE`
и одну целую дочернюю запись этого типа. Функция заимствует вход и
проверяет кадр, вид и диапазон целого. При `STYXRPC_OK` она записывает
`value`. При `STYXRPC_INVALID` выход не меняется. Функция не выполняет
I/O, выделение памяти или ожидание. См. [кодирование](value_integer.ru.md)
и [клиент на C](../examples/client.c).
