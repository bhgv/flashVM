# fvm_styxrpc_channel_delegate_pid

[English](STYX_RPC_CHANNEL_DELEGATE_PID.md) · [Объявление](../include/flashvm/styxrpc_channel.h) · [Реализация](../src/styxrpc/channel_delegate.c) · [Сценарий делегирования](STYX_RPC_CHANNEL_DELEGATION.ru.md)

Подключите `<flashvm/styxrpc_channel.h>` и линкуйте хостовую цель `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_channel_delegate_pid(FVM_Context *context, const FVM_Value *anchor,
    const StyxRpcChannelRoute *route, FVM_Value *out);
```

| Параметр | Контракт |
| --- | --- |
| `context` | Владелец живого контекста FlashVM и его значений. |
| `anchor` | Открытый канал результата RPC на том же двунаправленном монтировании, где хост опубликовал встречный ресурс. |
| `route` | Разрешённый хостом `STYXRPC_ROUTE_PID`. PID лежит в верхней половине 32-битного диапазона, токен пуст, ключ положительный 31-битный, схема элемента точная, а признак rendezvous равен 0 или 1. Маршрут копируется. |
| `out` | Обязательный выход для непрозрачного значения AS. Удержите корнем или присвойте его до следующего управляемого выделения. |

Возвращает [`FVM_OK`](../include/flashvm/flashvm.h#L44) при успехе. Неверные указатели, закрытая или отсутствующая аренда у `anchor` либо неверный маршрут дают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Ошибки памяти и корней передаются вызывающему. При ошибке выход сохраняется.

Используйте значение в подходящей позиции канала типизированного сообщения через ту же выдавшую аренду. Значение удерживает нативную аренду до GC и представляет опубликованную очередь хоста. У значения нет собственных методов канала. Встречную публикацию создаёт и отзывает хост. Ресурс должен оставаться активным, пока партнёр может его использовать. Вызывайте функцию на владельце FlashVM. Сетевой ввод-вывод при создании значения не выполняется.

[Публикация встречной группы](../../lib/styxrpc/reference/channel_group_reverse_output.ru.md) · [Исполняемая проверка хоста](../tests/styxrpc/channel-delegated.c)

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
| `fvm_styxrpc_channel_delegate_pid` | function | [include/flashvm/styxrpc_channel.h:27](../include/flashvm/styxrpc_channel.h#L27) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../include/flashvm/value.h#L26) |
<!-- source-links:end -->
