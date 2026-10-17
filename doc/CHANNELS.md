# FlashVM in-process channels (stage 27)

## Implemented API

`include/flashvm/channel.h` declares an opaque [`FVM_Channel`](../src/capi/channel_c.c#L26) and the functions [`fvm_channel_create`](../include/flashvm/channel.h#L23), [`fvm_channel_send`](../include/flashvm/channel.h#L53), [`fvm_channel_receive`](../include/flashvm/channel.h#L55), [`fvm_channel_available`](../include/flashvm/channel.h#L74), [`fvm_channel_close`](../include/flashvm/channel.h#L40), and [`fvm_channel_destroy`](../include/flashvm/channel.h#L49).

- Channels are unbounded, FIFO, and can be used by multiple producers and consumers. They are bidirectional in the sense that any participant can send or receive, but a message is consumed by exactly one receiver.
- `send` copies the value, including the bytes of a NUL-terminated string. Objects are rejected because object ownership and deep copying between VM contexts have not been implemented.
- `receive` blocks when empty, waking when data arrives or the channel closes. It returns [`FVM_ERROR`](../include/flashvm/flashvm.h#L43) if closed and empty. Queued messages can still be received after closing.
- `available` checks whether a queued message is present at the instant of the call. This is not a reservation: another receiver may consume it before the caller calls `receive`.
- The returned string pointer belongs to the channel and is valid until the next receive on that channel or destruction. Copy it if longer lifetime is required.
- Closing is thread-safe and wakes blocked receivers. Destroying is **not** safe while other threads are still calling channel functions. Join all users first.
- The implementation uses OS synchronization adapters under `ports/`. Linux is build-tested; Windows, macOS, and Android adapters require native platform testing.

## Not yet implemented

No worker VM creation, `thread.run`, `thread.join`, `thread.sleep`, channel sharing in tables, network channels, or ActionScript channel syntax is implemented by this stage. There is no bound on queue growth, no send backpressure, and no automatic cycle collection.

See `examples/embedding/channels/` for a compiled C89 example and an explicitly non-executable ActionScript syntax sketch.

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
