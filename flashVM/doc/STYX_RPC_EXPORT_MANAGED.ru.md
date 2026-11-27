# fvm_styxrpc_export_managed

[English](STYX_RPC_EXPORT_MANAGED.md) · [Заголовок](../include/flashvm/styxrpc_managed.h) · [Реализация](../src/styxrpc/managed.c) · [Мост](STYX_RPC.ru.md)

Подключите `flashvm/styxrpc_managed.h` и цель `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_export_managed(FVM_StyxRpcManagedExport *binding, const char *name,
    uint32_t signature, StyxRpcManagedExport *exported);
```

В `binding` укажите контекст-владелец, имя реализации AS, компилированную
схему и выделенный приложением буфер результата и обновлений на 1–65536 байтов.
Передайте сетевое имя `name`, компилированную сигнатуру `signature` и
память для описателя. [`FVM_OK`](../include/flashvm/flashvm.h#L44) означает, что проверенный синхронный
описатель заполнен. Неверный вход возвращает [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47),
выход `exported` сохраняется. Опубликуйте описатель через
[управляемый модуль](https://github.com/bhgv/flashVM/blob/master/lib/styxrpc/reference/managed_module_init.ru.md).

Обслуживайте колбэк в потоке владельца FlashVM. Он удерживает вновь
декодированные аргументы и результат временными GC-корнями и вызывает
именованную функцию AS. Неверный результат или исключение скрипта дают
удалённую ошибку. При уже ожидающем исключении вызов отклоняется.
Аргументы и результаты [кодируются по схеме](STYX_RPC_IMPORT_MANAGED.ru.md).
Изменения [обычных ref](STYX_RPC_REFS.ru.md) кодируются в том же буфере. Буфер результата
заимствуется до конца синхронного кодирования ответа. Он не должен
пересекаться с транспортными буферами. Описатель и данные привязки
должны жить без изменений до уничтожения сервера.

В позиции результата `big` принимается целый Number из точного диапазона или знаковый десятичный String. То же правило действует в массивах и кортежах. Вызывающая сторона Limbo получает знаковое 64-битное целое. [Справочник типов](STYX_RPC_VALUE_TYPES.ru.md)

[Исполняемая проверка](../tests/styxrpc/managed.c) · [Импорт](STYX_RPC_IMPORT_MANAGED.ru.md)

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
| `fvm_styxrpc_export_managed` | function | [include/flashvm/styxrpc_managed.h:32](../include/flashvm/styxrpc_managed.h#L32) |
| `FVM_StyxRpcManagedExport` | struct | [include/flashvm/styxrpc_managed.h:14](../include/flashvm/styxrpc_managed.h#L14) |
<!-- source-links:end -->
