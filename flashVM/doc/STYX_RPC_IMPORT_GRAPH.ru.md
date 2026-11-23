# fvm_styxrpc_import_graph

[English](STYX_RPC_IMPORT_GRAPH.md) · [Заголовок](../include/flashvm/styxrpc_graph.h) · [Реализация](../src/styxrpc/graph.c) · [Отображение графов](STYX_RPC_GRAPH.ru.md)

Подключите `flashvm/styxrpc_graph.h` и цель `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_import_graph(FVM_Context *context, const char *global,
    StyxRpcManagedTarget *target);
```

На выделенном владельце `context` зарегистрируйте синхронную глобальную функцию `global`. В `target` укажите смонтированную сессию, относительный путь, имя экспорта, подпись, обе схемы и ограничение времени ожидания ответа. Для экспорта с контекстом вызова задайте делегированный PID и сохраняйте его встречный контекст на протяжении вызова.

[`FVM_OK`](../include/flashvm/flashvm.h#L44) означает локальную регистрацию. Неверная настройка возвращает [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Ошибки аргументов, удалённой функции, транспорта и локальной записи передаются как исключения AS. Локальный отказ возможен после исполнения на другой стороне. Владелец, цель и её текстовые поля должны жить, пока функция доступна. Владелец может очистить поле сессии между вызовами перед закрытием.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_import_graph` | function | [include/flashvm/styxrpc_graph.h:16](../include/flashvm/styxrpc_graph.h#L16) |
<!-- source-links:end -->
