# fvm_styxrpc_channel_admit_module

[English](STYX_RPC_CHANNEL_ADMIT_MODULE.md) · [Объявление](../include/flashvm/styxrpc_channel.h) · [Реализация](../src/styxrpc/channel_delegate.c) · [Сценарий делегирования](STYX_RPC_CHANNEL_DELEGATION.ru.md)

Подключите `<flashvm/styxrpc_channel.h>` и линкуйте хостовую цель `flashvm_styxrpc`.

```c
FVM_Result fvm_styxrpc_channel_admit_module(FVM_Context *context, const FVM_Value *anchor,
    const StyxRpcChannelRoute *route, FVM_Value *out);
```

На владельце FlashVM `context` передайте открытый канал результата как `anchor`. Передайте проверенное хостом для этой аренды полномочие `STYXRPC_ROUTE_MODULE` с токеном, ключом, формой элемента и признаком rendezvous. Мост копирует маршрут. Последующие операции идут через модуль, удерживаемый арендой `anchor`. Присвойте или удержите корнем выход `out` до следующего управляемого выделения.

[`FVM_OK`](../include/flashvm/flashvm.h#L44) возвращает живой канал AS со штатными методами. Равная действующая пара токен–ключ возвращает тот же объект. Неверный вход, несовпадение схемы или признака rendezvous для существующей пары и заполнение таблицы дают [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47). Ошибка выделения или корней сохраняет `out`. Проверенный маршрут может остаться в аренде для повтора. Вызов работает без сетевого ввода-вывода. Следующая операция предъявляет токен на удерживаемом модуле.

Полученный канал удерживает исходный выдавший fid. Вызов `close()` освобождает эту обёртку и её алиасы. Хост связывает принятое проверенное сообщение делегирования с конкретной выдавшей арендой. [Допуск аренды C](../../lib/styxrpc/reference/channel_lease_admit_module.ru.md) · [Исполняемая проверка](../tests/styxrpc/channel-delegated.c)

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
| `fvm_styxrpc_channel_admit_module` | function | [include/flashvm/styxrpc_channel.h:54](../include/flashvm/styxrpc_channel.h#L54) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../include/flashvm/value.h#L26) |
<!-- source-links:end -->
