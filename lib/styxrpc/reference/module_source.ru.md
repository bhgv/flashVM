# styxrpc_module_source

[English](module_source.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/interface.h>
StyxRpcStatus styxrpc_module_source(const char *module, const StyxRpcDeclaration *functions, size_t count,
    const StyxRpcAdt *adts, size_t adt_count, char *bytes, size_t capacity, size_t *used);
```

Измеряет или выводит объявление модуля Limbo с завершающим NUL. Схема задаёт параметры и результат, описатели ADT — имена полей. Поддерживает массивы, кортежи, типизированные каналы и именованные ref ADT. Буфер отдельный, ёмкость больше used на NUL. Сигнатуру проверяет компилятор Limbo. Один токен формы соответствует одному имени ADT.

C99. `StyxRpc::rpc`.

[Пример](../examples/disvm/interface.c) · [Исходник](../src/interface.c)
