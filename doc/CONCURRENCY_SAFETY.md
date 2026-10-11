# Worker and channel ownership

Concurrent calls to `fvm_worker_join` on the same live handle are serialized.
Self-join returns `FVM_INVALID_ARGUMENT`. `FINISHED` is published only after the
private context and its finalizers have completed. Destroy remains an exclusive
owner operation: first join, wait for all other handle users, then destroy.
Cooperative cancellation cannot interrupt arbitrary native callback code.

`fvm_channel_receive_owned(channel, &value, blocking)` transfers each string's
allocation to its consumer. Release exactly once with
`fvm_channel_message_release`; the value remains valid across other receives and
channel destruction. This is the API for concurrent string consumers. Legacy
receive remains borrowed and requires external synchronization until the consumer
has finished reading. `try_send` returns `FVM_NOT_FOUND` on a full open queue;
close returns `FVM_ERROR` for sends and wakes both producers and consumers. Queued
messages can still be drained. Retain before publishing a handle; every active
user must hold ownership until its operations complete.

Primitive value tags are validated before worker creation or channel enqueue.
Object pointers are rejected. Linux is tested; other adapters require native
platform CI. OS thread identity and microsecond sleep live in `ports/`; sleep is
best effort and Windows rounds up to milliseconds.

Tests: `worker_join_concurrent` uses condition variables to coordinate two
joiners and reject self-join without timing assumptions; `channel_owned` checks
independent string lifetime, backpressure, close and post-destruction release.
