# Worker/channel integration (C89 API)

Each `FVM_Worker` creates an independent Runtime and Context on a native OS thread. Its callback receives that context as the first argument. Ordinary AVM1 object pointers must never cross the context boundary. Host code may instead pass pointers to separately created `FVM_Channel` instances in the callback's `userdata`; that pointer must remain valid until `join` completes.

## Message lifetime and errors

`fvm_channel_send` copies primitive values and NUL-terminated UTF-8 strings. It rejects object values. `fvm_channel_receive` waits until a message arrives or the channel is closed. `fvm_channel_try_receive` never waits: `FVM_OK` means a message was consumed, `FVM_NOT_FOUND` means open and empty, and `FVM_ERROR` means closed and empty. Both receive functions return borrowed string pointers, valid until the next successful receive on the same channel or channel destruction. A single channel is a FIFO queue, **not** a paired endpoint; use two channels for independent request and response directions.

`fvm_worker_stop` is cooperative and does not interrupt a blocked `receive`. The owner should close the appropriate channel to wake a blocked worker before joining it. The host must ensure no thread is accessing a channel when it is destroyed. Channel ownership and reference counting across contexts are not implemented yet.

See `examples/embedding/worker_channels/` for a complete tested C89 program. This is a host/native API; ActionScript thread/channel syntax remains planned.
