# Independent resource observers

The C89 `revents.h` API extends the native dispatcher with coherent discovery,
independent event queues and session cleanup. It does not open sockets, mount a
filesystem or deserialize VM pointers. A transport authenticates each session;
the owner grants rights. [Dispatcher](RESOURCE_DISPATCH.md),
[namespace plan](RESOURCE_NAMESPACE.md), [tutorial](../examples/embedding/resource_events/README.md).

## API and ownership

All functions borrow the dispatcher for the call. Outputs change only on OK;
native records must be encoded field by field before crossing an ABI or network.
The existing host lock serializes jobs, grants and observers together. Its hooks
must remain valid until destroy; callbacks must not reenter while holding it.
Without lock hooks all access must stay on one thread or be externally serialized.
No allocation after dispatcher creation, VM call or GC root is introduced here.

| Function | Contract |
| --- | --- |
| [`fvm_rdispatch_resources`](../include/flashvm/revents.h#L42) | Takes session, rows, capacity and required count pointer. NULL rows/zero capacity queries count. A non-NULL short buffer returns LIMIT without touching rows/count. Snapshot includes only currently granted resources, copied canonical paths and operations whose required rights the session holds. No subtree inheritance or implicit directory traversal. |
| [`fvm_rdispatch_subscribe`](../include/flashvm/revents.h#L47) | Takes session, resource incarnation and watch output. Requires SUBSCRIBE. Each successful call allocates a distinct empty native queue; missing resource UNKNOWN, missing right DENIED, exhausted slots LIMIT. No resource callback is executed. |
| [`fvm_rdispatch_publish`](../include/flashvm/revents.h#L52) | Owner-only: resource, nonzero Uint32 kind, counted payload and sequence output. Copies at most [FVM_R_BYTES](../include/flashvm/rdispatch.h#L9) to each matching subscriber. Excess bytes LIMIT; NULL nonempty payload INVALID. Sequence starts at one and never wraps; exhaustion LIMIT has no publication effect. No receiver still consumes a sequence. |
| [`fvm_rdispatch_event_take`](../include/flashvm/revents.h#L59) | Any synchronized consumer supplies its authenticated session/watch and event output. Copies and removes only that reader's oldest event. PENDING means empty; a stale watch is UNKNOWN and a different session is DENIED. On OK `missed` reports that reader's drops since its previous take, then resets. |
| [`fvm_rdispatch_unsubscribe`](../include/flashvm/revents.h#L63) | Session/watch close one queue and discard its unread events. Other readers remain intact. Stale watch UNKNOWN; another session DENIED. |
| [`fvm_rdispatch_disconnect`](../include/flashvm/revents.h#L69) | Owner-only, after the session transport has quiesced. Atomically removes grants, observers, queued jobs and retained replies for that session. Running work from the same session returns BUSY before any change. Other producers retain FIFO order. It never undoes executed side effects. |

[`FVM_RResource`](../include/flashvm/revents.h#L31), [`FVM_RWatch`](../include/flashvm/revents.h#L13) and [`FVM_REvent`](../include/flashvm/revents.h#L19) are copied native records. Resource
and watch handles have separate incarnation spaces. A path is a name, not an
authority token. SUBSCRIBE can be granted independently from READ and CONTROL;
the host decides whether an event's contents are appropriate for that audience.

## Loss, revocation and reconnect

There are [FVM_R_WATCHES](../include/flashvm/revents.h#L5) queues, each retaining [FVM_R_EVENT_QUEUE](../include/flashvm/revents.h#L6) events. A slow
reader drops its own oldest copy; another reader's progress is unaffected. Loss
is explicit in `missed`, saturating at Uint32 max. The global sequence orders
publications across resources, so a numeric gap alone does not prove loss for
one resource. It is not a wall clock or a distributed total-order guarantee.

Removing SUBSCRIBE immediately discards the matching session/resource queues;
regrant does not restore their data. Unregister invalidates all watches of that
resource before the slot can be reused. Exhausted watch generations retire their
slots. Dispatcher destroy discards remaining observer copies after users are
quiesced; accepted command replies still require take/disconnect as before.

Disconnect must also prevent late transport packets from being accepted. This
API cannot infer reconnection epochs: allocate fresh authenticated session IDs
or reject stale wire epochs in the adapter. A reply discarded at disconnect may
already have committed effects; request IDs are not durable exactly-once keys.

This is application fan-out, useful for GUI observers and debugger stop events.
A local UDP multicast/broadcast adapter or an authenticated relay may carry it;
no internet multicast, 9P, transport authentication or media stream is provided.

## Verification and build

```sh
cmake -S flashVM -B build-events -DFVM_BUILD_EXAMPLES=ON
cmake --build build-events --target flashvm_revents_contract flashvm_revents_threads flashvm_examples_embedding_resource_events_main
ctest --test-dir build-events -R '^flashvm_revents_' --output-on-failure
```

The thread target is native POSIX only; omit it on Windows/cross builds. The C89
library itself does not depend on pthreads: hosts supply their own lock hooks.
Contract tests cover filtering, output atomicity, independent loss accounting,
revocation/regrant, generation and sequence exhaustion, running-session BUSY and
FIFO cleanup. Three actual POSIX consumers each reconcile 501 publications as
received plus explicitly missed. These tests do not certify foreign OS runtime,
network delivery, real GUI controls or final roadmap acceptance.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_R_BYTES` | macro / define | [include/flashvm/rdispatch.h:9](../include/flashvm/rdispatch.h#L9) |
| `FVM_R_EVENT_QUEUE` | macro / define | [include/flashvm/revents.h:6](../include/flashvm/revents.h#L6) |
| `FVM_R_WATCHES` | macro / define | [include/flashvm/revents.h:5](../include/flashvm/revents.h#L5) |
| `fvm_rdispatch_disconnect` | function | [include/flashvm/revents.h:69](../include/flashvm/revents.h#L69) |
| `fvm_rdispatch_event_take` | function | [include/flashvm/revents.h:59](../include/flashvm/revents.h#L59) |
| `fvm_rdispatch_publish` | function | [include/flashvm/revents.h:52](../include/flashvm/revents.h#L52) |
| `fvm_rdispatch_resources` | function | [include/flashvm/revents.h:42](../include/flashvm/revents.h#L42) |
| `fvm_rdispatch_subscribe` | function | [include/flashvm/revents.h:47](../include/flashvm/revents.h#L47) |
| `fvm_rdispatch_unsubscribe` | function | [include/flashvm/revents.h:63](../include/flashvm/revents.h#L63) |
| `FVM_REvent` | struct | [include/flashvm/revents.h:19](../include/flashvm/revents.h#L19) |
| `FVM_RResource` | struct | [include/flashvm/revents.h:31](../include/flashvm/revents.h#L31) |
| `FVM_RWatch` | struct | [include/flashvm/revents.h:13](../include/flashvm/revents.h#L13) |
<!-- source-links:end -->
