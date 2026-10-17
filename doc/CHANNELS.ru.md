# FlashVM: внутренние каналы (этап 27)

## Реализованный API

В `include/flashvm/channel.h` объявлен непрозрачный тип [`FVM_Channel`](../src/capi/channel_c.c#L26) и функции [`fvm_channel_create`](../include/flashvm/channel.h#L23), [`fvm_channel_send`](../include/flashvm/channel.h#L53), [`fvm_channel_receive`](../include/flashvm/channel.h#L55), [`fvm_channel_available`](../include/flashvm/channel.h#L74), [`fvm_channel_close`](../include/flashvm/channel.h#L40), [`fvm_channel_destroy`](../include/flashvm/channel.h#L49).

- Канал — неограниченная FIFO-очередь, допускающая несколько отправителей и получателей. Каждый участник может читать и писать, но каждое сообщение получает только один читатель.
- `send` копирует значение, в том числе строку до завершающего нулевого байта. Объекты не принимаются: глубокое копирование объектов между контекстами VM ещё не реализовано.
- `receive` блокируется при пустой очереди и пробуждается при поступлении сообщения или закрытии канала. Если канал закрыт и пуст, возвращается [`FVM_ERROR`](../include/flashvm/flashvm.h#L43). Уже поставленные в очередь сообщения после закрытия доступны.
- `available` проверяет наличие сообщения в момент вызова, но не резервирует его для вызывающего потока.
- Полученная строка принадлежит каналу и действительна до следующего чтения из этого канала или уничтожения канала. При необходимости сохраните копию.
- `close` потокобезопасен и пробуждает ожидающих читателей. `destroy` нельзя вызывать, пока другие потоки используют канал: сначала завершите и объедините эти потоки.
- Синхронизация вынесена в `ports/`. Сборка проверена на Linux; Windows, macOS и Android требуют отдельного тестирования.

## Пока не реализовано

Запуск независимых VM в потоках, `thread.run`, `thread.join`, `thread.sleep`, передача каналов внутри таблиц, сетевые каналы и синтаксис каналов в ActionScript отсутствуют. Нет ограничения размера очереди, обратного давления при отправке и автоматического сборщика циклов.

См. `examples/embedding/channels/`: рабочий пример на C89 и эскиз будущего синтаксиса ActionScript, который пока не выполняется.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Channel` | struct | [src/capi/channel_c.c:26](../src/capi/channel_c.c#L26) |
| `fvm_channel_available` | function | [include/flashvm/channel.h:74](../include/flashvm/channel.h#L74) |
| `fvm_channel_close` | function | [include/flashvm/channel.h:40](../include/flashvm/channel.h#L40) |
| `fvm_channel_create` | function | [include/flashvm/channel.h:23](../include/flashvm/channel.h#L23) |
| `fvm_channel_destroy` | function | [include/flashvm/channel.h:49](../include/flashvm/channel.h#L49) |
| `fvm_channel_receive` | function | [include/flashvm/channel.h:55](../include/flashvm/channel.h#L55) |
| `fvm_channel_send` | function | [include/flashvm/channel.h:53](../include/flashvm/channel.h#L53) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
<!-- source-links:end -->
