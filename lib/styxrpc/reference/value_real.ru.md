# styxrpc_value_real

[English](value_real.md) · [Заголовок](../include/styxrpc/managed.h) · [Реализация](../src/managed.c)

Подключите `styxrpc/managed.h` и цель `StyxRpc::rpc`.

```c
StyxRpcStatus styxrpc_value_real(double value, void *bytes, size_t capacity, size_t *used);
```

Закодировать binary64 `value` как действующую дочернюю запись
real Dis, включая знак нуля и нагрузку NaN. `bytes` — буфер вызывающего
размером `capacity` байтов. При `STYXRPC_OK` в `used` выдаётся длина
кадра. При `STYXRPC_INVALID` переданный `used` обнуляется. Неверные
аргументы, недостаточный буфер и неподдерживаемая раскладка double
отклоняются без I/O и выделения памяти.

[Проверка обмена](../tests/managed.c)
