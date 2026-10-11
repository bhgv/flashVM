# FlashVM in-process channels (stage 27)

## Implemented API

`include/flashvm/channel.h` declares an opaque `FVM_Channel` and the functions `fvm_channel_create`, `fvm_channel_send`, `fvm_channel_receive`, `fvm_channel_available`, `fvm_channel_close`, and `fvm_channel_destroy`.

- Channels are unbounded, FIFO, and can be used by multiple producers and consumers. They are bidirectional in the sense that any participant can send or receive, but a message is consumed by exactly one receiver.
- `send` copies the value, including the bytes of a NUL-terminated string. Objects are rejected because object ownership and deep copying between VM contexts have not been implemented.
- `receive` blocks when empty, waking when data arrives or the channel closes. It returns `FVM_ERROR` if closed and empty. Queued messages can still be received after closing.
- `available` checks whether a queued message is present at the instant of the call. This is not a reservation: another receiver may consume it before the caller calls `receive`.
- The returned string pointer belongs to the channel and is valid until the next receive on that channel or destruction. Copy it if longer lifetime is required.
- Closing is thread-safe and wakes blocked receivers. Destroying is **not** safe while other threads are still calling channel functions. Join all users first.
- The implementation uses OS synchronization adapters under `ports/`. Linux is build-tested; Windows, macOS, and Android adapters require native platform testing.

## Not yet implemented

No worker VM creation, `thread.run`, `thread.join`, `thread.sleep`, channel sharing in tables, network channels, or ActionScript channel syntax is implemented by this stage. There is no bound on queue growth, no send backpressure, and no automatic cycle collection.

See `examples/embedding/channels/` for a compiled C89 example and an explicitly non-executable ActionScript syntax sketch.
