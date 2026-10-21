# Resource namespace and message dispatch design

[Русский](RESOURCE_NAMESPACE.ru.md) · [Binary records](SRECORD.md) · [Roadmap](ROADMAP_STATUS.md).

This records the accepted architecture for future FlashVM resource control.
**Implemented now:** binary S-record framing, directional type declarations,
fragmented reception and a native/file tutorial. **Not implemented here:** a
resource dispatcher, remote GUI methods, a mounted filesystem, 9P server or full
Inferno namespace behavior. Existing VM channel serialization remains unchanged.

## Shared operation layer

Three layers share one operation model: typed C calls, permission-aware request/
reply/event dispatch, then Limbo-channel/IPC/network/filesystem adapters. Local
C callers need not encode/decode messages; a remote adapter cannot bypass the
same argument, ownership and authority checks. Schema names/versions identify
operations and values; numeric tags are direction/session-local associations.
An unknown schema is not a license to invoke a named C function dynamically.

| Proposed path | Meaning |
| --- | --- |
| `/scene/root/children/` | Named child resource directories |
| `/scene/root/transform` | Per-open transform snapshot / typed update |
| `/scene/root/ctl` | Clip commands, with request and result identity |
| `/scene/root/events` | Independent per-open subscription |
| `/timeline/ctl` | Commands supported by the actual owner API |
| `/audio/channels/17/ctl` | Authorized sound-channel control |
| `/display/frame` | Completed frame metadata/chunks |
| `/input/events` | Explicit injected input, not raw native pointers |
| `/ui/windows/main/children/save/` | Widget resource directory |

Each widget is a directory with `value`, `properties`, `ctl`, `events` and
`children` endpoints as applicable. Files expose capabilities of a resource;
creating/deleting ordinary backing directories must not silently create/destroy
VM objects. API discovery must report unsupported operations honestly: naming
seek/video/filter endpoints cannot add missing SWF/runtime capabilities.

## Ownership, isolation and concurrency

An addressed resource uses a stable ID and generation; its path is discoverable
naming, not permanent identity. Delete/recreate cannot target the replacement
through an old handle. Every request carries a session request ID, target handle,
operation schema, arguments and optional expected state version/deadline. Replies
distinguish accepted/queued from committed/failed. A retry requires bounded
request deduplication or an explicit indeterminate result after session loss;
exactly-once effects across arbitrary crashes are not promised.

Each resource has an execution owner. Network and filesystem workers parse,
authenticate and queue immutable copied commands; they never call VM/render/SDL
on the wrong thread. A local session may route different subtrees to different
owners, including separate processes/machines. Work targeting one VM remains
serialized at its owner safe points. Local multi-property batches can be atomic
when all preconditions pass; distributed atomicity requires another protocol.
No remote client holds the render-thread mutex while waiting for a reply.

[GC:] Remote handles are capability references, not C pointers or serialized
object addresses. Pin/lookup/unpin belongs to the VM owner. Queued native work
must use copied values or explicit owner-managed leases; disconnect/timeout
releases subscriptions/leases and rejects stale generations. Graph snapshots
must preserve cycles/shared references with IDs, while live remote references
remain explicit capabilities. Native audio/window/socket resources need resource
schemas, not memory-layout copies. This series does not yet implement that graph
or lease machinery.

Authority is separate from names and unguessable-looking IDs. Export an allowed
subtree and operation set per client: e.g. one panel writable, audio read-only,
frame subscription only. Directory traversal/aliases/binds cannot escape the
export. Authentication, confidentiality, quotas and authorization are transport/
dispatch responsibilities; short adaptive checksums are not their replacement.

## Ordinary open/read/write integration

The protocol API deliberately accepts bytes and caller buffers, so an adapter
can consume partial `read`/`write` operations without depending on packet or file
call boundaries. A future namespace client library exposes open/read/write/close
style handles on every platform. Making these paths visible to arbitrary programs
through their normal OS filesystem calls additionally needs a real platform
mount/redirector, such as a suitable 9P/FUSE adapter; it is not provided by the
codec or by ordinary fopen of a local transcript.

Define endpoint semantics before exposing mounts:

- Property reads use per-open stable snapshots, making offsets/partial reads
  meaningful. Refresh/reopen policy must be explicit.
- Control writes assemble complete checked records. A successful byte write
  acknowledges input consumption, not necessarily completed VM effects. Read
  results/events with request IDs for execution acknowledgement.
- Events are per-open bounded streams with sequence numbers, cancellation and
  explicit overflow/gap reporting. One reader must not consume another's events.
- Frame/audio/video endpoints use versioned binary chunk schemas with stream ID,
  sequence, byte/chunk extent, timestamp units and discontinuity/end markers.
  Control traffic needs independent quotas/flow control so media cannot starve STOP.
- Closing/flushing cancels pending delivery according to declared state; already
  committed commands are not rolled back. Mount/client retries must not duplicate
  non-idempotent effects merely because a byte write is repeated.

Prepare namespace semantics separately from the wire adapter: per-client roots,
attach/bind/mount/unmount, union ordering and collision rules, path encoding,
parent traversal at export boundaries, permissions, stat/discovery, live handles
across rename/unlink, cancellation and mount teardown. Desired Inferno behavior
and intentional deviations need a compatibility matrix and tests before a claim
of Inferno/9P compatibility. Native filesystem names and payload type identifiers
are different domains; the metadata ASCII identity limit is not a path encoding.

## Incremental implementation order

1. Completed in this series: canonical framing, atomic bindings, incremental
   input, direct C linkage and binary/file examples.
2. Next: bounded owner dispatcher, request/reply/event schemas, handle generations,
   subtree authority and operation discovery, tested without VM side effects.
3. Wire timeline/control/frame services through existing typed owner APIs,
   then clip/drawing/audio/input capabilities with GC/cancel/error tests.
4. Implement a filesystem-like client API and adapters; add real mounts/9P only
   with protocol conformance and native OS acceptance. Extend to Inferno namespace
   features against an explicit tested semantic contract.
