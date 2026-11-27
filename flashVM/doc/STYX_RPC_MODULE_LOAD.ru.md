# fvm_styxrpc_module_load

[English](STYX_RPC_MODULE_LOAD.md) · [API](../include/flashvm/styxrpc_module.h) · [Source](../src/styxrpc/module_import.c) · [Guide](STYX_RPC_MODULES.ru.md)

`flashvm_styxrpc`

```c
FVM_Result fvm_styxrpc_module_load(FVM_Context *context,
    const FVM_StyxRpcLoadSpec *spec, FVM_Value *out);
```

## Контракт

Передайте контекст владельца, подключённую сессию, относительный путь, компилированные импорты, PID для профилей с контекстом, срок вызова в миллисекундах и необязательную шину аргументов. Вызов ждёт проверки интерфейса и копирует контракт в объект AS. Успех даёт [`FVM_OK`](../include/flashvm/flashvm.h#L44) и объект в `out`. Сбой удалённой проверки даёт [`FVM_NOT_FOUND`](../include/flashvm/flashvm.h#L49). Возможна локальная ошибка проверки или выделения. Удерживайте объект обычными средствами GC и сохраняйте сессию и шину на время вызовов.

Прокси владеет неоткрытым fid [захваченного каталога](../../lib/styxrpc/reference/module_open.ru.md).
Каждый метод проходит путь от этого fid. После отзыва образа старые методы
отклоняются до отправки запроса, даже если владелец повторно опубликовал те же
имена и сигнатуры. Каталог освобождается после исчезновения последних ссылок
на прокси и извлечённые методы.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `FVM_NOT_FOUND` | enum constant | [include/flashvm/flashvm.h:49](../include/flashvm/flashvm.h#L49) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_module_load` | function | [include/flashvm/styxrpc_module.h:76](../include/flashvm/styxrpc_module.h#L76) |
| `FVM_StyxRpcLoadSpec` | struct | [include/flashvm/styxrpc_module.h:62](../include/flashvm/styxrpc_module.h#L62) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../include/flashvm/value.h#L26) |
<!-- source-links:end -->
