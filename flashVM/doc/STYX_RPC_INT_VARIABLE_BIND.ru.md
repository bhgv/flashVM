# fvm_styxrpc_int_variable_bind

[English](STYX_RPC_INT_VARIABLE_BIND.md) · [Руководство](STYX_RPC_FILESYSTEM.ru.md)

```c
#include <flashvm/styxrpc_filesystem.h>
FVM_Result fvm_styxrpc_int_variable_bind(FVM_Context *context, FVM_Object *object, const char *name,
    struct StyxClient *session, const char *path, int writable);
```

Линкуйте `flashvm_styxrpc`. В потоке-владельце привяжите новое свойство `name` объекта `object` к нативной 32-битной знаковой переменной по относительному `path` на смонтированной `session`. Файл должен объявлять длину четыре байта. Укажите `writable=0` для чтения или 1 для чтения и записи. Точный fid удерживается, пока свойство хранит его GC-корень. Каждое чтение получает свежее значение. Присваивание проверяет точный int и передаёт одно полное обновление. Подтверждённые и неопределённые сбои транспорта вызывают исключения. Неопределённая запись автоматически не повторяется. Неверные аргументы и повторное свойство дают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47), ошибка открытия файла — [`FVM_ERROR`](../include/flashvm/flashvm.h#L45).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_Object` | struct | [src/core/runtime.hpp:133](../src/core/runtime.hpp#L133) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_int_variable_bind` | function | [include/flashvm/styxrpc_filesystem.h:29](../include/flashvm/styxrpc_filesystem.h#L29) |
<!-- source-links:end -->
