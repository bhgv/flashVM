# Worker/channel integration (C89 API)

Each [`FVM_Worker`](../src/capi/worker_c.c#L14) creates an independent Runtime and Context on a native OS thread. Its callback receives that context as the first argument. Ordinary AVM1 object pointers must never cross the context boundary. Host code may instead pass pointers to separately created [`FVM_Channel`](../src/capi/channel_c.c#L30) instances in the callback's `userdata`; that pointer must remain valid until `join` completes.

## Message lifetime and errors

[`fvm_channel_send`](../include/flashvm/channel.h#L106) copies primitive values and NUL-terminated UTF-8 strings. It rejects object values. [`fvm_channel_receive`](../include/flashvm/channel.h#L110) waits until a message arrives or the channel is closed. [`fvm_channel_try_receive`](../include/flashvm/channel.h#L133) never waits: [`FVM_OK`](../include/flashvm/flashvm.h#L44) means a message was consumed, [`FVM_NOT_FOUND`](../include/flashvm/flashvm.h#L49) means open and empty, and [`FVM_ERROR`](../include/flashvm/flashvm.h#L45) means closed and empty. Both receive functions return borrowed string pointers, valid until the next successful receive on the same channel or channel destruction. A single channel is a FIFO queue, **not** a paired endpoint; use two channels for independent request and response directions.

[`fvm_worker_stop`](../include/flashvm/worker.h#L69) is cooperative and does not interrupt a blocked `receive`. The owner should close the appropriate channel to wake a blocked worker before joining it. The host must ensure no thread is accessing a channel when it is destroyed. Channel ownership and reference counting across contexts are not implemented yet.

See `examples/embedding/worker_channels/` for a complete tested C89 program. This is a host/native API; ActionScript thread/channel syntax remains planned.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Channel` | struct | [src/capi/channel_c.c:30](../src/capi/channel_c.c#L30) |
| `fvm_channel_receive` | function | [include/flashvm/channel.h:110](../include/flashvm/channel.h#L110) |
| `fvm_channel_send` | function | [include/flashvm/channel.h:106](../include/flashvm/channel.h#L106) |
| `fvm_channel_try_receive` | function | [include/flashvm/channel.h:133](../include/flashvm/channel.h#L133) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_NOT_FOUND` | enum constant | [include/flashvm/flashvm.h:49](../include/flashvm/flashvm.h#L49) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_Worker` | struct | [src/capi/worker_c.c:14](../src/capi/worker_c.c#L14) |
| `fvm_worker_stop` | function | [include/flashvm/worker.h:69](../include/flashvm/worker.h#L69) |
<!-- source-links:end -->
