# styxrpc_value_get_real

[English](value_get_real.md) · [Заголовок](../include/styxrpc/managed.h) · [Реализация](../src/managed.c)

Подключите `styxrpc/managed.h` и цель `StyxRpc::rpc`.

```c
StyxRpcStatus styxrpc_value_get_real(const void *bytes, size_t size, double *value);
```

Декодировать одну полную запись real Dis из `bytes` длиной `size`.
Функция сверяет тег, кадр и вид записи и сохраняет биты binary64 без
числового преобразования. `STYXRPC_OK` записывает `value`.
`STYXRPC_INVALID` сохраняет выход, в том числе на неподдерживаемой
раскладке double. Вход заимствуется. I/O, ожидания и выделения памяти нет.

[Проверка обмена](../tests/managed.c)
