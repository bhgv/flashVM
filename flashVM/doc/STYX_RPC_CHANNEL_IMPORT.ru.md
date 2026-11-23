# fvm_styxrpc_channel_import

[English](STYX_RPC_CHANNEL_IMPORT.md) · [Объявление](../include/flashvm/styxrpc_channel.h) · [Реализация](../src/styxrpc/channel_import.c) · [Методы каналов](STYX_RPC_CHANNELS.ru.md)

Подключите `flashvm/styxrpc_channel.h` и цель `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_channel_import(FVM_Context *context, const char *global,
    StyxRpcManagedTarget *target);
```

На владельце `context` установите глобальную функцию AS с именем `global`. В `target` передайте смонтированную сессию, точные имена модуля и экспорта, подпись, схему, делегированный ненулевой PID и время опроса в миллисекундах. Дескриптор и его строки живут, пока вызываема функция. Сессия должна быть активна во время вызова. Между вызовами можно изменить цель. Уже возвращённые каналы сохраняют исходную аренду и собственный срок опроса.

[`FVM_OK`](../include/flashvm/flashvm.h#L44) сообщает о регистрации. Отсутствующие или неверные аргументы дают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Остальные ошибки регистрации передаются вызывающему. Вызов проверяет профиль, обменивается одним кадром и создаёт локальные объекты каналов. Ошибка после отправки возможна после исполнения на удалённой стороне. Явный `close()` закрывает аренду синхронно при последней ссылке. GC может отложить освобождение. Остановка заимствованной сессии снимает её ожидающие операции.

[Исполняемая проверка](../tests/styxrpc/channel.c)

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
| `fvm_styxrpc_channel_import` | function | [include/flashvm/styxrpc_channel.h:18](../include/flashvm/styxrpc_channel.h#L18) |
<!-- source-links:end -->
