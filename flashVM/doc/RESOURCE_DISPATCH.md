# Bounded owner resource dispatcher

[Русский](RESOURCE_DISPATCH.ru.md) · [Namespace plan](RESOURCE_NAMESPACE.md) ·
[Binary records](SRECORD.md) · [C89 declarations](../include/flashvm/rdispatch.h).

The dispatcher implements copied control requests, explicit grants and owner-side
FIFO execution. It is independent of VM/SDL/transport and can be linked from
`flashvm_srecord` or the VM SDK. No worker, socket, filesystem mount or VM context
is implicitly created. Registered resources are exact absolute leaf paths;
this first registry does not implement directories, aliases or subtree exports.

Each resource has a slot ID and a nonzero 32-bit generation. Unregister advances
generation without wrapping; exhausted slots retire. Queued old handles finish
UNKNOWN instead of invoking replacement userdata. IDs are local to one dispatcher
lifetime and are not transferable capability tokens. Stable sessions must not
replay old handles into a new dispatcher without a new namespace/session epoch.

The server copies up to eight operation descriptions per resource. Each specifies
its required READ/WRITE/CONTROL/SUBSCRIBE right; a client cannot label a write as
a read to bypass authority. Grants associate exact resource generation with an
adapter-authenticated session. Post and execution both check current authority.
Revocation rejects work not yet executing; it does not stop/roll back running code.
Session integers passed to C are trusted adapter inputs, not authentication by
themselves. Never trust a remote packet's self-declared session identity.

## Ownership and synchronization

Create copies the optional policy. Without lock/unlock hooks, all access is
single-owner and externally serialized. With working host mutex hooks, producers
may post/cancel/take concurrently with one owner pump. Lock/unlock must both be
present or both absent, must work without failure and must not reenter dispatcher.
The optional is_owner callback rejects register/unregister/grant/pump/destroy
from other threads; without it, owner-thread use remains a caller precondition.
Policy userdata/mutex outlives destroy and every native caller must be quiesced
before freeing the dispatcher. This is not a raw-pointer lifetime reclamation API.

Resource callbacks execute outside the queue lock, on the pumping owner. Register
borrows callback userdata; unregister fails BUSY while that handler runs. It may
release queued registrations, since queued jobs retain identity and copied bytes,
not userdata pointers. [GC:] A registration does not pin a managed object. VM
adapters must acquire/release managed roots on the VM owner and keep resources
alive until successful unregister/destroy. Do not pass VM pointers in requests.

Callbacks may enqueue other work/query results, but nested pump and destruction
are BUSY. A successful handler return means the handler completed its own contract;
it does not universally mean a frame was presented or a distributed effect became
atomic. Exceptions/longjmp across C and writes beyond reply capacity are prohibited.
An invalid reported extent/status yields HANDLER after already committed effects.

## Queue and result semantics

There are hard bounds: 16 resource slots, 64 grants, 32 accepted requests and
512 bytes per request/reply. This is a control plane, not a media stream buffer.
Queued/running/completed results share request capacity. Full post returns LIMIT
without effects. Each accepted request copies bytes and retains its session/ID
until take. Duplicates before take return CONFLICT; **after take the same ID can
execute again**. An authenticated transport must define acknowledgement/replay
retention; exactly-once delivery after crashes/reconnects is not supplied here.

Post OK means accepted, not executed. Pump executes up to the explicit budget in
FIFO order. Handler errors are retained replies, not failure to pump. Cancel
unlinks queued work and produces CANCELLED; running cancellation returns BUSY,
and cancellation after completion CONFLICT. Take copies/removes completed reply;
PENDING/UNKNOWN/error leaves caller output unchanged. No callback-owned pointer
escapes through take. Destroy returns BUSY while any accepted result is unacknowledged.
Host userdata/mutex are never freed by the dispatcher.

| C API | Purpose and publication |
| --- | --- |
| [`fvm_rdispatch_create`](../include/flashvm/rdispatch.h#L95) | Validate/copy policy and allocate one bounded native object; publish output only on OK, NOMEM otherwise. |
| [`fvm_rdispatch_register`](../include/flashvm/rdispatch.h#L99) | Owner copies canonical path/operation table, borrows handler userdata and returns a fresh handle. |
| [`fvm_rdispatch_unregister`](../include/flashvm/rdispatch.h#L105) | Owner releases registration/grants; executing handler BUSY, queued old handles become stale. |
| [`fvm_rdispatch_grant`](../include/flashvm/rdispatch.h#L108) | Owner creates/replaces exact session authority; zero revokes and frees an entry. |
| [`fvm_rdispatch_lookup`](../include/flashvm/rdispatch.h#L111) | Exact path lookup with current authority; copy handle only on OK. No ambient filesystem lookup. |
| [`fvm_rdispatch_post`](../include/flashvm/rdispatch.h#L115) | Producers copy a bounded request after authority/identity/capacity checks. No handler runs here. |
| [`fvm_rdispatch_pump`](../include/flashvm/rdispatch.h#L119) | Owner delivers at most budget callbacks unlocked and retains their copied results. No recursive pumping. |
| [`fvm_rdispatch_cancel`](../include/flashvm/rdispatch.h#L121) | Session-scoped queued cancellation; does not interrupt a callback. |
| [`fvm_rdispatch_take`](../include/flashvm/rdispatch.h#L126) | Copy and acknowledge completed result; releases identity and queue capacity. |
| [`fvm_rdispatch_destroy`](../include/flashvm/rdispatch.h#L131) | Quiesced owner frees only dispatcher when no accepted requests remain; nulls caller pointer. |

Paths are bytewise case-sensitive, bounded and reject empty segments, dot/dot-dot,
backslashes/control bytes and trailing slash. Non-ASCII bytes are opaque; no UTF-8
normalization or OS filesystem mapping is implied. All native input/output storage
must be accessible and disjoint from dispatcher internals. Caller supplies valid
C strings; native structs are never their own wire encoding.

## Build

```sh
cmake -S flashVM -B build-resource -DFVM_BUILD_TESTS=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/minimal.defs"
cmake --build build-resource --target flashvm_srecord flashvm_rdispatch_contract --parallel
ctest --test-dir build-resource -R '^flashvm_rdispatch_' --output-on-failure
```

No SDL/network SDK is needed. Cross builds supply the target toolchain and libc;
tests execute natively on their target. Native platform/device/VM/GUI integration
and full Inferno/9P conformance remain open.
## Binary envelopes and real producers (v0.727)

[rwire.h](../include/flashvm/rwire.h) connects the native queue to S-records without
serializing C structs. First declare a direction-local tag as
flashvm.resource.request/version 1 or flashvm.resource.reply/version 1. Numbers
are canonical big-endian S integers; payload bytes are opaque and bounded to 512.

```text
request payload: version, request-id, target-slot, generation, operation-id, size, bytes
reply payload:   version, request-id, handler-result, size, bytes
```

There is **no session ID on wire**: post_record takes the authenticated identity
from a trusted adapter argument. A client cannot select another session by
encoding extra fields. A resource operation ID is not the S-record tag. CRC and
type-name/schema-version checks complete before any request is enqueued. Unknown
bindings are UNKNOWN, invalid metadata/CRC INVALID and incomplete prefixes PENDING;
none call handlers. No type declaration is automatically accepted by this helper.

| API | Contract |
| --- | --- |
| [`fvm_rrequest_record_write`](../include/flashvm/rwire.h#L20) | Encode a bounded native request into caller output using its chosen bound tag; deliberately ignores native session field. No queue effects. S status/atomic output. |
| [`fvm_rdispatch_post_record`](../include/flashvm/rwire.h#L28) | Verify the incoming record/binding/payload, then copy to queue with authenticated session. consumed changes only on accepted post. On a framed rejection, transport must discard/respond explicitly rather than retry forever. |
| [`fvm_rreply_record_write`](../include/flashvm/rwire.h#L33) | Serialize an owned native reply without taking a result; preserve that native copy for output/I/O retries. No automatic queue acknowledgement. |
| [`fvm_rreply_record_read`](../include/flashvm/rwire.h#L39) | Verify incoming reply binding/schema and copy id/result/bytes only on OK. No callback or queue effect; all outputs unchanged on failure. |

These helpers do not implement sockets, authentication, reconnect deduplication,
subscription routing, request cancellation packets or 9P. Native cancellation
remains an explicit trusted C operation. Transport code owns complete-record
framing/consumption and directional table lifetime.

A separate POSIX test uses four real producer threads and an owner loop, 200
copied requests, native mutex policy, explicit backpressure and acknowledged
replies. Workers are denied owner pump. The handler reacquires the nonrecursive
mutex to prove pump released it. This is native Linux thread evidence, not a
claim of tested foreign OS threads or a distributed mutex implementation.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_rdispatch_cancel` | function | [include/flashvm/rdispatch.h:121](../include/flashvm/rdispatch.h#L121) |
| `fvm_rdispatch_create` | function | [include/flashvm/rdispatch.h:95](../include/flashvm/rdispatch.h#L95) |
| `fvm_rdispatch_destroy` | function | [include/flashvm/rdispatch.h:131](../include/flashvm/rdispatch.h#L131) |
| `fvm_rdispatch_grant` | function | [include/flashvm/rdispatch.h:108](../include/flashvm/rdispatch.h#L108) |
| `fvm_rdispatch_lookup` | function | [include/flashvm/rdispatch.h:111](../include/flashvm/rdispatch.h#L111) |
| `fvm_rdispatch_post` | function | [include/flashvm/rdispatch.h:115](../include/flashvm/rdispatch.h#L115) |
| `fvm_rdispatch_post_record` | function | [include/flashvm/rwire.h:28](../include/flashvm/rwire.h#L28) |
| `fvm_rdispatch_pump` | function | [include/flashvm/rdispatch.h:119](../include/flashvm/rdispatch.h#L119) |
| `fvm_rdispatch_register` | function | [include/flashvm/rdispatch.h:99](../include/flashvm/rdispatch.h#L99) |
| `fvm_rdispatch_take` | function | [include/flashvm/rdispatch.h:126](../include/flashvm/rdispatch.h#L126) |
| `fvm_rdispatch_unregister` | function | [include/flashvm/rdispatch.h:105](../include/flashvm/rdispatch.h#L105) |
| `fvm_rreply_record_read` | function | [include/flashvm/rwire.h:39](../include/flashvm/rwire.h#L39) |
| `fvm_rreply_record_write` | function | [include/flashvm/rwire.h:33](../include/flashvm/rwire.h#L33) |
| `fvm_rrequest_record_write` | function | [include/flashvm/rwire.h:20](../include/flashvm/rwire.h#L20) |
<!-- source-links:end -->
