# Portable resource files

The `rfs.h` C89 API exposes dispatcher resources through synthetic directories
and files. It is callable from an IDE, player or another C/C++ program without
a console, OS mount, socket, worker or VM dependency. It links into
`flashvm_srecord` as well as the core SDK. [Events](RESOURCE_EVENTS.md),
[dispatcher](RESOURCE_DISPATCH.md), [tutorial](../examples/embedding/resource_files/README.md).

## Namespace and identity

Root contains one `slot.generation` directory for each resource visible to the
authenticated session. Each has `path` (original registration path bytes), `ops`
(a directory of decimal server operation IDs), and `events` when SUBSCRIBE is
granted. Example: `/1.1/path`, `/1.1/ops/7`, `/1.1/events`. Discover actual names;
never assume a particular allocation order outside the fresh tutorial instance.
Leading zeroes, signs, slash aliases and ambient OS traversal are rejected.
`..` walks directories; root's parent is root. There are no explicit dot entries.

This initial identity tree is not yet the proposed named scene/widget tree,
an Inferno namespace, a mount implementation or complete 9P server. A later
adapter can project registered paths without confusing an old incarnation with
a replacement. Native entries are copied; serialize each field, never structs.
Qid halves identify root or a resource generation plus an immutable server
operation position. They remain stable across sessions and rights filtering
within the same dispatcher lifetime; there is no persistence across restart.

## API and ownership

Every function is nonblocking and requires external serialization of this FS
instance. The borrowed dispatcher can independently use its existing lock hooks.
There are no managed pointers or GC roots. Discovery/authority checks linearize
at dispatcher lock acquisitions; revocation does not interrupt a running handler.
Outputs are unchanged on error unless explicitly stated. Native input/output
storage must not overlap the filesystem's opaque storage or other output objects.

| Function | Contract |
| --- | --- |
| [`fvm_rfs_create`](../include/flashvm/rfs.h#L49) | Dispatcher, authenticated nonzero Uint32 session, output pointer. Allocates one fixed native object; NOMEM leaves output intact. Borrow dispatcher until destroy. This session's request-ID namespace must be exclusive to this instance, including other direct dispatcher callers; no rights or authentication are created. |
| [`fvm_rfs_stat`](../include/flashvm/rfs.h#L51) | FS, canonical node, entry output. Rechecks current incarnation and rights; UNKNOWN for absent/invisible resources, DENIED for an unauthorized child, INVALID malformed node. Returns copied metadata, never a borrowed object. |
| [`fvm_rfs_walk`](../include/flashvm/rfs.h#L54) | FS, parent directory node, single NUL-terminated component and entry output. Parent is checked first. Supports `..`, canonical resource names and authorized children; leaf traversal INVALID. No allocation, I/O or VM call. |
| [`fvm_rfs_open`](../include/flashvm/rfs.h#L58) | FS, node, exact supported mode and descriptor output. Directories/PATH/EVENTS require READ; CALL requires READ|WRITE. Allocates one of [FVM_RFS_FILES](../include/flashvm/rfs.h#L5) native slots; events also allocate an independent dispatcher watch. Wrong mode DENIED, capacity LIMIT; errors publish no descriptor. |
| [`fvm_rfs_readdir`](../include/flashvm/rfs.h#L62) | FS, open directory, zero-based entry index, output. Returns one copied entry; PENDING means end. Captured discovery remains stable; any resource/grant/operation-filter change gives CONFLICT until reopen. This includes unrelated visible resources and prevents a revoked entry leaking from a stale snapshot. |
| [`fvm_rfs_read`](../include/flashvm/rfs.h#L67) | FS, descriptor, Uint32 offset, nonempty output capacity and count output. PATH allows arbitrary offsets, including EOF. CALL/EVENTS require exact next offset; PENDING means no result/event yet. Partial records remain owned by the descriptor; errors leave output/count unchanged. Zero capacity INVALID. Directories use readdir instead. |
| [`fvm_rfs_write`](../include/flashvm/rfs.h#L73) | FS, CALL descriptor, offset zero, counted payload and count output. Exactly one request per open, including empty payload; at most [FVM_R_BYTES](../include/flashvm/rdispatch.h#L9). Copies and queues with a monotonically assigned Uint32 request ID. OK is acceptance, never execution; repeated write CONFLICT, exhausted IDs LIMIT. A failed post leaves descriptor/count unchanged. |
| [`fvm_rfs_close`](../include/flashvm/rfs.h#L79) | FS and descriptor. Cancels queued work, acknowledges its retained reply and closes its own observer; completed side effects remain. Running handler BUSY keeps the descriptor open for retry. Stale descriptor UNKNOWN. A revoked/removed watch or request is tolerated during cleanup. No session disconnect or VM destruction. |
| [`fvm_rfs_destroy`](../include/flashvm/rfs.h#L82) | Pointer to FS pointer after users quiesce. Any open descriptor BUSY. Success frees only native FS storage and nulls pointer; dispatcher, grants and registrations remain host-owned. |
| [`fvm_rdispatch_operation_index`](../include/flashvm/revents.h#L48) | Dispatcher, session, resource incarnation, operation ID, index output. Checks authority under its mutex and returns immutable zero-based server table position. UNKNOWN resource/operation, DENIED missing right; no queue mutation. This position is not a permission or an index into filtered discovery. |
| [`fvm_rdispatch_watch_check`](../include/flashvm/revents.h#L69) | Dispatcher, session and watch. Checks incarnation/ownership without consuming data. Revoked/recreated watches UNKNOWN, another session DENIED. Used before returning retained event bytes after a partial read. |

## Calls, streams and cleanup

Write raw operation payload bytes, then explicitly pump the dispatcher on its
owner. A read waits with PENDING until the terminal reply exists. Output begins
with a tag-zero declaration binding [FVM_RFS_TAG](../include/flashvm/rfs.h#L17) to `flashvm.resource.reply`,
followed by one checksummed typed reply record. Read in pieces using cumulative
offsets; after its last byte the call file returns EOF. Reopen for another call.
The request ID is retained until the reply is copied into the descriptor, then
the dispatcher queue slot is acknowledged. Closing before that cancels queued
work or acknowledges completed work; closing after that discards unread bytes.
No retry deduplication beyond dispatcher retention is promised.

An event open has its own queue. Its stream begins with one tag-zero declaration
for `flashvm.resource.event`, followed by complete checked records; a read may
split them into pieces. No event yet means PENDING, never false EOF. Slow readers
have independent missed counts. Even already buffered bytes require the original
watch incarnation: revoke/regrant cannot resurrect an old stream. At the Uint32
stream limit return LIMIT before dequeuing another event; close/reopen starts a
fresh stream. Records and their metadata use the canonical S integers and CRC
profile described in [binary framing](SRECORD.md); no native struct is on wire.

Open directories own coherent snapshots. On CONFLICT close/reopen and restart
listing. Close every descriptor, quiesce users, destroy the FS, then disconnect
its session on the owner if appropriate, and finally destroy the dispatcher.
Never destroy the dispatcher while an FS still borrows it. Per-instance bounds
are [FVM_RFS_FILES](../include/flashvm/rfs.h#L5) descriptors and one [FVM_RFS_BUFFER](../include/flashvm/rfs.h#L7) per descriptor; observers
also share the dispatcher's [FVM_R_WATCHES](../include/flashvm/revents.h#L5) bound. No allocations follow create.

## Build and evidence

```sh
cmake -S flashVM -B build-files -DFVM_BUILD_TESTS=ON -DFVM_BUILD_EXAMPLES=ON
cmake --build build-files --target flashvm_rfs_contract flashvm_examples_embedding_resource_files_main
ctest --test-dir build-files -R '^flashvm_rfs_' --output-on-failure
```

The C89 contract checks fragmented replies, independent events, revoke/regrant,
stable qids across filtered sessions, directory invalidation, stale generations,
queued cancellation, running-close BUSY, empty payload, fixed-slot/ID exhaustion,
allocation failure and atomic outputs. This is native protocol evidence, not
foreign-OS execution, complete Flash compatibility, a network listener or mount.
[9P2000 message adapter](RESOURCE_9P.md) added in v0.733; OS mount and the named scene tree remain separate work.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_R_BYTES` | macro / define | [include/flashvm/rdispatch.h:9](../include/flashvm/rdispatch.h#L9) |
| `FVM_R_WATCHES` | macro / define | [include/flashvm/revents.h:5](../include/flashvm/revents.h#L5) |
| `fvm_rdispatch_operation_index` | function | [include/flashvm/revents.h:48](../include/flashvm/revents.h#L48) |
| `fvm_rdispatch_watch_check` | function | [include/flashvm/revents.h:69](../include/flashvm/revents.h#L69) |
| `FVM_RFS_BUFFER` | macro / define | [include/flashvm/rfs.h:7](../include/flashvm/rfs.h#L7) |
| `fvm_rfs_close` | function | [include/flashvm/rfs.h:79](../include/flashvm/rfs.h#L79) |
| `fvm_rfs_create` | function | [include/flashvm/rfs.h:49](../include/flashvm/rfs.h#L49) |
| `fvm_rfs_destroy` | function | [include/flashvm/rfs.h:82](../include/flashvm/rfs.h#L82) |
| `FVM_RFS_FILES` | macro / define | [include/flashvm/rfs.h:5](../include/flashvm/rfs.h#L5) |
| `fvm_rfs_open` | function | [include/flashvm/rfs.h:58](../include/flashvm/rfs.h#L58) |
| `fvm_rfs_read` | function | [include/flashvm/rfs.h:67](../include/flashvm/rfs.h#L67) |
| `fvm_rfs_readdir` | function | [include/flashvm/rfs.h:62](../include/flashvm/rfs.h#L62) |
| `fvm_rfs_stat` | function | [include/flashvm/rfs.h:51](../include/flashvm/rfs.h#L51) |
| `FVM_RFS_TAG` | macro / define | [include/flashvm/rfs.h:17](../include/flashvm/rfs.h#L17) |
| `fvm_rfs_walk` | function | [include/flashvm/rfs.h:54](../include/flashvm/rfs.h#L54) |
| `fvm_rfs_write` | function | [include/flashvm/rfs.h:73](../include/flashvm/rfs.h#L73) |
<!-- source-links:end -->
