# fvm_styxrpc_export_graph

[English](STYX_RPC_EXPORT_GRAPH.md) · [Заголовок](../include/flashvm/styxrpc_graph.h) · [Реализация](../src/styxrpc/graph.c) · [Отображение графов](STYX_RPC_GRAPH.ru.md)

Подключите `flashvm/styxrpc_graph.h` и цель `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_export_graph(FVM_StyxRpcManagedExport *binding, const char *name,
    uint32_t signature, StyxRpcGraphExport *exported);
```

Подготовьте дескриптор для компилированного имени `name` и подписи `signature`. В `binding` задайте владельца FlashVM, имя функции AS, полную схему и буфер результата до 64 КиБ. Опубликуйте `exported` через [графовый модуль libstyxrpc](../../lib/styxrpc/reference/graph_module_init.ru.md). Обслуживайте сервер на владельце контекста FlashVM. Привязка, имена и память живут до завершения работы сервера. Вызовы с одним буфером выполняются последовательно на владельце.

[`FVM_OK`](../include/flashvm/flashvm.h#L44) означает, что дескриптор подготовлен. [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) сообщает о неверной привязке. Исключение AS или ошибка преобразования результата передаются удалённому вызывающему как ошибка.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_export_graph` | function | [include/flashvm/styxrpc_graph.h:22](../include/flashvm/styxrpc_graph.h#L22) |
| `FVM_StyxRpcManagedExport` | struct | [include/flashvm/styxrpc_managed.h:14](../include/flashvm/styxrpc_managed.h#L14) |
<!-- source-links:end -->
