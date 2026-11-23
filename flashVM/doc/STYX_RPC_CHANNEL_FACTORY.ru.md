# fvm_styxrpc_channel_factory

[English](STYX_RPC_CHANNEL_FACTORY.md) · [Объявление](../include/flashvm/styxrpc_channel.h) · [Реализация](../src/styxrpc/channel_local.c) · [Сценарий аргументов](STYX_RPC_CHANNEL_ARGUMENTS.ru.md)

Подключите `flashvm/styxrpc_channel.h` и цель `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_channel_factory(FVM_Context *context, const char *global);
```

Зарегистрируйте непустое имя `global` на владельце контекста FlashVM. [`FVM_OK`](../include/flashvm/flashvm.h#L44) сообщает об успешной регистрации. Отсутствующий контекст или пустое имя дают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Ошибки нативной регистрации передаются вызывающему. Конструктор принимает ровно два аргумента, компактный тип по значению, например `"i"` для int или `"s"` для строки, и точное число слотов от 0 до 8. При успехе он возвращает объект FlashVM. Неверный вход даёт [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47), нехватка памяти — [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L46).

`trySend(value)` возвращает true, если элемент добавлен, и false при заполнении. `tryReceive()` извлекает типизированный элемент или возвращает `undefined` для пустой очереди. Несовпадение типа вызывает ошибку AS. Очередь нулевой ёмкости локально пуста и может синхронизировать операции Dis во время RPC. Повторное `close()` допустимо. Оно отключает все алиасы. Объект принадлежит одному владельцу, его память освобождает GC. Для передачи очереди используйте [привязку аргументов](STYX_RPC_CHANNEL_ARGUMENTS_IMPORT.ru.md). Возобновляйте локальные операции после возврата вызова.

[Исполняемая проверка](../tests/styxrpc/channel-args.c)

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
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_styxrpc_channel_factory` | function | [include/flashvm/styxrpc_channel.h:37](../include/flashvm/styxrpc_channel.h#L37) |
<!-- source-links:end -->
