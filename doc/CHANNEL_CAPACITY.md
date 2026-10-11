# Bounded channels (C API)

`fvm_channel_set_capacity(channel, count)` sets the maximum number of queued messages. A value of zero (the default) means unbounded. When the queue reaches capacity, `fvm_channel_send()` blocks until a consumer receives a message or the channel is closed. `fvm_channel_pending()` returns a synchronized snapshot of the queue length. `fvm_channel_close()` wakes blocked senders and receivers; blocked sends return `FVM_ERROR` after closure. Already queued messages remain readable.

These functions are available to C89 and C++98 callers. Do not destroy a channel while any thread uses it. For predictable backpressure, configure the capacity before starting producers. No ActionScript channel operators are implemented by this change.
