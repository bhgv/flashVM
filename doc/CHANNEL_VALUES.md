# Owned channel handles: API and limitations

## API

`FVM_ChannelValue` is a small C89 handle containing `FVM_Channel *`.
`fvm_channel_value_create` creates an internal channel with one reference.
`fvm_channel_value_from_channel` retains an existing channel and puts its
pointer in the destination handle. `fvm_channel_value_copy` retains the
source handle's channel; it does **not** duplicate messages or queue state.
`fvm_channel_value_release` relinquishes one reference and clears the handle.
`fvm_channel_value_send` and `fvm_channel_value_receive` delegate to the
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

This handle is **not** an `FVM_Value` type or a GC-traced AVM1 value. It cannot
be stored as an ordinary ActionScript property, serialized inside arbitrary
AVM1 tables, or passed by the proposed `<...>` syntax. Native `userdata` is
not copied by worker creation; ownership is the host's responsibility.
See `examples/embedding/channel_values`, `channel_cpp98` and
`examples/extending/channel_worker`.
