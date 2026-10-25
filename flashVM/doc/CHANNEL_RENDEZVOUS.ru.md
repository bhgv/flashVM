# Блокирующие каналы и наблюдение за ожидающими сторонами

[English](CHANNEL_RENDEZVOUS.md) · [Публичный API](../include/flashvm/channel.h) ·
[Реализация](../src/capi/channel_c.c) · [Общий хост Dis](../../disVM/host/README.ru.md)

Обычный локальный канал по умолчанию остаётся FIFO с копированием значений.
[`fvm_channel_receive`](../include/flashvm/channel.h#L110) ждёт данные; [`fvm_channel_send`](../include/flashvm/channel.h#L106) ждёт при заполнении заданной
ёмкости. `try_send`/`try_receive` не ждут партнёра. Блокирующий вызов на потоке
VM/отрисовки остановит этот поток: используйте подходящий worker.

`fvm_channel_set_rendezvous(channel, 1)` включает встречу отправителя и получателя
на пустом, открытом канале без ожидающих операций. Отправитель выставляет одно
предложенное сообщение и ждёт, пока получатель заберёт именно его. Ёмкость не
добавляет буфер в этом режиме. Поддерживаются несколько отправителей/получателей,
но гарантии справедливого порядка пробуждений нет. Неблокирующая отправка требует
уже ожидающего получателя и пустого, не замороженного слота; иначе возвращается
[`FVM_NOT_FOUND`](../include/flashvm/flashvm.h#L49). Успешный try-send фиксирует предложение готовой стороне, не ожидая
возобновления её нативного потока. Получать можно через borrowed/owned/graph API;
примитивный приём по-прежнему не может забрать графовый пакет.

Закрытие будит обе стороны. Получатель без доступного предложения получает
[`FVM_ERROR`](../include/flashvm/flashvm.h#L45). Если блокирующая отправка ещё не принята, она снимает предложение
и возвращает [`FVM_ERROR`](../include/flashvm/flashvm.h#L45); при ошибке отправки графа владение остаётся у вызывающего.
Если приём уже состоялся, отправка успешна даже при последующем close. Старые
буферизированные очереди после закрытия продолжают отдавать накопленные сообщения.
Каждый владелец удерживает канал до завершения своей операции/потока; destroy и
последний release не должны гоняться с пользователями. Заморозка отладчика
останавливает приём; thaw или close пробуждает ожидающих.

`fvm_channel_wait_state(channel, &state)` возвращает согласованный снимок под
мьютексом, не ожидая данных, свободного места или партнёра. Краткий захват мьютекса
есть: API не является lock-free. Снимок содержит число сообщений, ожидающих
отправителей и получателей, флаги закрытия/заморозки/режима и биты
[`FVM_CHANNEL_WAIT_SEND`](../include/flashvm/channel.h#L21) / [`FVM_CHANNEL_WAIT_RECEIVE`](../include/flashvm/channel.h#L22). Оба бита могут присутствовать
одновременно. Считаются нативные ожидания и сбалансированные отменяемые polling-
ожидания ActionScript. Это наблюдение, а не резервирование операции, идентификатор
конкретного потока или гарантия успешности следующего try-вызова.

Существующие [`fvm_channel_available`](../include/flashvm/channel.h#L139) / `pending` показывают наличие данных.
При заморозке данные могут быть в очереди, но ещё не доступны для выдачи.
На AS-обёртке добавлены числовые методы `waitingSenders()`, `waitingReceivers()`
и `waitDirections()`. Они не ждут партнёра. Сохранены `ready()`, `pending()`,
`trySend()` и `tryReceive()`, отмена ожиданий и учёт отладчика. Режим rendezvous
в этой серии включает хост; отдельного AS-переключателя и нового remote-opcode нет.

Снимок ожиданий и переключение режима поддержаны для локальных очередей.
Файловые и удалённые каналы возвращают [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L48), не меняя выходной снимок:
локальная обёртка не должна выдумывать сведения о потоках удалённой стороны.
Переключение непустого/занятого/закрытого канала даёт [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47).
Нативные структуры и указатели не попадают в wire-формат. Это расширение не
является заявлением о совместимости Styx/9P.

Проверки покрывают отправителя первым, получателя первым, try-отказы, подтверждение
приёма, отмену при close, ожидание ёмкости FIFO и 800 передач между четырьмя
нативными потоками. Проверяются прежние channel/owned/capacity/shared/close/value/
graph и AS extension сценарии. Исполнение проверено на Linux; Windows/macOS/Android
используют соответствующие порты FlashVM и ещё требуют целевых запусков.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_channel_available` | function | [include/flashvm/channel.h:139](../include/flashvm/channel.h#L139) |
| `fvm_channel_receive` | function | [include/flashvm/channel.h:110](../include/flashvm/channel.h#L110) |
| `fvm_channel_send` | function | [include/flashvm/channel.h:106](../include/flashvm/channel.h#L106) |
| `fvm_channel_set_rendezvous` | function | [include/flashvm/channel.h:48](../include/flashvm/channel.h#L48) |
| `FVM_CHANNEL_WAIT_RECEIVE` | macro / define | [include/flashvm/channel.h:22](../include/flashvm/channel.h#L22) |
| `FVM_CHANNEL_WAIT_SEND` | macro / define | [include/flashvm/channel.h:21](../include/flashvm/channel.h#L21) |
| `fvm_channel_wait_state` | function | [include/flashvm/channel.h:41](../include/flashvm/channel.h#L41) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_NOT_FOUND` | enum constant | [include/flashvm/flashvm.h:49](../include/flashvm/flashvm.h#L49) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
<!-- source-links:end -->
