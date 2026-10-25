# Owned channel handles: API and limitations

## API

[`FVM_ChannelValue`](../include/flashvm/channel_value.h#L17) is a small C89 handle containing `FVM_Channel *`.
[`fvm_channel_value_create`](../include/flashvm/channel_value.h#L24) creates an internal channel with one reference.
[`fvm_channel_value_from_channel`](../include/flashvm/channel_value.h#L28) retains an existing channel and puts its
pointer in the destination handle. [`fvm_channel_value_copy`](../include/flashvm/channel_value.h#L32) retains the
source handle's channel; it does **not** duplicate messages or queue state.
[`fvm_channel_value_release`](../include/flashvm/channel_value.h#L37) relinquishes one reference and clears the handle.
[`fvm_channel_value_send`](../include/flashvm/channel_value.h#L41) and [`fvm_channel_value_receive`](../include/flashvm/channel_value.h#L46) delegate to the
existing blocking channel operations.

### `fvm_channel_value_create`

`out` must point to an unused handle. Creates an unbounded in-process queue with one owned reference. Returns [`FVM_OK`](../include/flashvm/flashvm.h#L44), [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) for NULL, or [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L46) on allocation/synchronization creation failure, or [`FVM_ERROR`](../include/flashvm/flashvm.h#L45) if channel identity allocation fails. A valid out has channel set to NULL on failure.

### `fvm_channel_value_from_channel`

`out` is an unused destination; `channel` is a live channel. Retains one additional reference and writes out only on success. Returns [`FVM_OK`](../include/flashvm/flashvm.h#L44) or [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) for NULL or reference-count overflow. Does not consume the original reference. Leaves out unchanged on failure.

### `fvm_channel_value_copy`

`out` is a separate unused destination; `source` is a live owning handle. Retains the same channel without duplicating its queue or messages. Returns [`FVM_OK`](../include/flashvm/flashvm.h#L44) or [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) for NULL/empty source or reference-count overflow. Leaves out unchanged on failure. Do not self-copy or overwrite a live destination: the extra retained reference would be lost.

### `fvm_channel_value_release`

`value` identifies the reference to release. Returns void; NULL and empty handles are harmless. Clears channel after releasing one reference. The final release destroys the channel and queued messages; other threads must have finished their operations first. This is not close: remaining owners keep an open channel open.

### `fvm_channel_value_send`

`value` is a live handle; `message` is a primitive value. In-process queues copy the value and string bytes without consuming the original. Waits while a bounded queue is full until space is available or it closes. Returns [`FVM_OK`](../include/flashvm/flashvm.h#L44), [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) for invalid pointers/type/NULL string, [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L48) for a metatable, [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L46) on allocation failure, or [`FVM_ERROR`](../include/flashvm/flashvm.h#L45) for a closed queue. File/remote channels delegate to underlying send and propagate serialization/I/O errors.

### `fvm_channel_value_receive`

`value` is a live handle; `message` is a non-NULL output. Waits for an in-process message, including while frozen. A closed queue can still yield pending primitives. Returns [`FVM_OK`](../include/flashvm/flashvm.h#L44), [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47) for invalid pointers, [`FVM_ERROR`](../include/flashvm/flashvm.h#L45) for a closed empty/frozen queue, or [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L48) for a graph packet at the head (not consumed). Read the output only after [FVM_OK](../include/flashvm/flashvm.h#L44). Strings are borrowed until the next successful receive on the same channel or destruction; retaining another handle does not protect a string across receives. Multiple consumers should use owned receive in the channel API. File/remote channels propagate underlying receive statuses; file EOF does not wait for an external writer.

These wrappers do not enter a VM, invoke ActionScript or establish GC roots. Queue synchronization protects channel internals, not concurrent modification of the same handle. Keep the handle alive for the entire operation; coordinate borrowed strings between consumers.

## Ownership and threading

Each successful create/from/copy requires one release. The destination must
be uninitialized or previously released: overwriting an active handle leaks
its reference. Do not concurrently release the final reference while another
thread is operating on that handle. `worker_join` must complete before
releasing handles passed through `userdata`. Closing the channel wakes blocked
senders and receivers. A received string is borrowed and valid only until
another receive on the same channel or destruction.

## Not yet implemented

This handle is **not** an [`FVM_Value`](../include/flashvm/value.h#L26) type or a GC-traced AVM1 value. It cannot
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
| `FVM_Channel` | struct | [src/capi/channel_c.c:30](../src/capi/channel_c.c#L30) |
| `fvm_channel_value_copy` | function | [include/flashvm/channel_value.h:32](../include/flashvm/channel_value.h#L32) |
| `fvm_channel_value_create` | function | [include/flashvm/channel_value.h:24](../include/flashvm/channel_value.h#L24) |
| `fvm_channel_value_from_channel` | function | [include/flashvm/channel_value.h:28](../include/flashvm/channel_value.h#L28) |
| `fvm_channel_value_receive` | function | [include/flashvm/channel_value.h:46](../include/flashvm/channel_value.h#L46) |
| `fvm_channel_value_release` | function | [include/flashvm/channel_value.h:37](../include/flashvm/channel_value.h#L37) |
| `fvm_channel_value_send` | function | [include/flashvm/channel_value.h:41](../include/flashvm/channel_value.h#L41) |
| `FVM_ChannelValue` | struct | [include/flashvm/channel_value.h:17](../include/flashvm/channel_value.h#L17) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../include/flashvm/value.h#L26) |
<!-- source-links:end -->
