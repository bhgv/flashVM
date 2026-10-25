# Bounded channels (C API)

`fvm_channel_set_capacity(channel, count)` sets the maximum number of queued messages. A value of zero (the default) means unbounded. When the queue reaches capacity, `fvm_channel_send()` blocks until a consumer receives a message or the channel is closed. `fvm_channel_pending()` returns a synchronized snapshot of the queue length. `fvm_channel_close()` wakes blocked senders and receivers; blocked sends return [`FVM_ERROR`](../include/flashvm/flashvm.h#L45) after closure. Already queued messages remain readable.

These functions are available to C89 and C++98 callers. Do not destroy a channel while any thread uses it. For predictable backpressure, configure the capacity before starting producers. No ActionScript channel operators are implemented by this change.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_channel_close` | function | [include/flashvm/channel.h:85](../include/flashvm/channel.h#L85) |
| `fvm_channel_pending` | function | [include/flashvm/channel.h:151](../include/flashvm/channel.h#L151) |
| `fvm_channel_send` | function | [include/flashvm/channel.h:106](../include/flashvm/channel.h#L106) |
| `fvm_channel_set_capacity` | function | [include/flashvm/channel.h:146](../include/flashvm/channel.h#L146) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
<!-- source-links:end -->
