# styxrpc_program_collection

[English](program_collection.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/collection.h>
int styxrpc_program_collection(StyxRpcPrograms *programs, uint32_t pid, const char *name, StyxIntArray *array);
```

Одной регистрацией публикует .idx, .val, .type, .len и доступные мутации. Поддерживает позиционные коллекции или коллекции Int32 с числовыми ключами. Откройте .idx, запишите u32 в сетевом порядке, затем откройте .val на том же соединении. Удерживайте .idx до конца работы с .val. Мутации требуют закрытого селектора. .ins и .add принимают ключ/индекс и int32, .push принимает один int32, .pop читает сохранённое на fid значение из 4 байт. Состояние колбэков живёт до закрытия всех fid.

`StyxRpc::serve`.

[Пример](../examples/disvm/reverse.c) · [Исходник](../src/program.c)
