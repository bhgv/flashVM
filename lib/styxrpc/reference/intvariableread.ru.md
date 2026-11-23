# styxintvariableread

[English](intvariableread.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/variable_file.h>
int styxintvariableread(const StyxIntVariable *variable, uint64_t offset, void *bytes, size_t *size);
```

Читает один целый знаковый int32 через заимствованный колбэк load. На входе size задаёт ёмкость в байтах, на выходе четыре байта в сетевом порядке или ноль для EOF со смещения четыре и далее. При нулевой ёмкости возвращает ноль байт. Неверное смещение, короткий буфер или ошибка колбэка дают код 0. Доступ к состоянию согласует приложение.

C99. `StyxRpc::rpc`.

[Пример](../examples/disvm/reverse.c) · [Исходник](../protocol/variable_file.c)
