# Owned channel handles: API and limitations

## API

[`FVM_ChannelValue`](../include/flashvm/channel_value.h#L17) is a small C89 handle containing `FVM_Channel *`.
[`fvm_channel_value_create`](../include/flashvm/channel_value.h#L22) creates an internal channel with one reference.
[`fvm_channel_value_from_channel`](../include/flashvm/channel_value.h#L24) retains an existing channel and puts its
pointer in the destination handle. [`fvm_channel_value_copy`](../include/flashvm/channel_value.h#L26) retains the
source handle's channel; it does **not** duplicate messages or queue state.
[`fvm_channel_value_release`](../include/flashvm/channel_value.h#L29) relinquishes one reference and clears the handle.
[`fvm_channel_value_send`](../include/flashvm/channel_value.h#L31) and [`fvm_channel_value_receive`](../include/flashvm/channel_value.h#L34) delegate to the
existing blocking channel operations.

## Ownership and threading

Each successful create/from/copy requires one release. The destination must
be uninitialized or previously released: overwriting an active handle leaks
its reference. Do not concurrently release the final reference while another
thread is operating on that handle. `worker_join` must complete before
releasing handles passed through `userdata`. Closing the channel wakes blocked
senders and receivers. A received string is borrowed and valid only until
another receive on the same channel or destruction.

## Not yet implemented

This handle is **not** an [`FVM_Value`](../include/flashvm/value.h#L24) type or a GC-traced AVM1 value. It cannot
be stored as an ordinary ActionScript property, serialized inside arbitrary
AVM1 tables, or passed by the proposed `<...>` syntax. Native `userdata` is
not copied by worker creation; ownership is the host's responsibility.
See `examples/embedding/channel_values`, `channel_cpp98` and
`examples/extending/channel_worker`.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Channel` | struct | [src/capi/channel_c.c:26](../src/capi/channel_c.c#L26) |
| `fvm_channel_value_copy` | function | [include/flashvm/channel_value.h:26](../include/flashvm/channel_value.h#L26) |
| `fvm_channel_value_create` | function | [include/flashvm/channel_value.h:22](../include/flashvm/channel_value.h#L22) |
| `fvm_channel_value_from_channel` | function | [include/flashvm/channel_value.h:24](../include/flashvm/channel_value.h#L24) |
| `fvm_channel_value_receive` | function | [include/flashvm/channel_value.h:34](../include/flashvm/channel_value.h#L34) |
| `fvm_channel_value_release` | function | [include/flashvm/channel_value.h:29](../include/flashvm/channel_value.h#L29) |
| `fvm_channel_value_send` | function | [include/flashvm/channel_value.h:31](../include/flashvm/channel_value.h#L31) |
| `FVM_ChannelValue` | struct | [include/flashvm/channel_value.h:17](../include/flashvm/channel_value.h#L17) |
| `FVM_Value` | struct | [include/flashvm/value.h:24](../include/flashvm/value.h#L24) |
<!-- source-links:end -->
