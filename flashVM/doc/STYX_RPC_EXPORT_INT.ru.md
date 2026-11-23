# fvm_styxrpc_export_int

[English](STYX_RPC_EXPORT_INT.md) · [Объявление](../include/flashvm/styxrpc.h) · [Реализация](../src/styxrpc/scalar.c) · [Подключение моста](STYX_RPC.ru.md)

Подключите `flashvm/styxrpc.h`, `styxrpc/scalar_module.h` и цель `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_export_int(FVM_StyxRpcExport *binding, const char *name,
    struct StyxRpcExport *exported);
```

В `binding` укажите контекст-владелец и имя функции скрипта. Передайте имя
сетевого экспорта `name` и память `exported`. [`FVM_OK`](../include/flashvm/flashvm.h#L44) означает, что записан
описатель `fn(int): int`. При неверных параметрах возвращается
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47), описатель сохраняется. Опубликуйте его через
[API скалярного модуля](https://github.com/bhgv/flashVM/blob/master/lib/styxrpc/reference/scalar_module_init.ru.md).

Обслуживайте сервер в потоке владельца контекста FlashVM. Колбэк на каждом
вызове разрешает имя функции и передаёт аргумент как число AS. Результат
должен быть точным знаковым Int32. Исключение скрипта, отсутствие функции или
неверный результат дают запись с удалённой ошибкой. Колбэк снимает возникшее
в этом вызове исключение, сохраняя ранее ожидающее исключение.
Храните контекст, описатель, binding и имена до уничтожения сервера.
Сервер владеет состояниями fid и выданным транспортом.

[Исполняемая проверка](../tests/styxrpc_host.c) · [Импорт](STYX_RPC_IMPORT_INT.ru.md)

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
| `fvm_styxrpc_export_int` | function | [include/flashvm/styxrpc.h:48](../include/flashvm/styxrpc.h#L48) |
| `FVM_StyxRpcExport` | struct | [include/flashvm/styxrpc.h:25](../include/flashvm/styxrpc.h#L25) |
<!-- source-links:end -->
