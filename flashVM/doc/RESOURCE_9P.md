# 9P2000 resource adapter

`r9p.h` is an original bounded C89 message server over the
[resource filesystem](RESOURCE_FILES.md). It is included in flashvm_srecord and
the core SDK. A host supplies authenticated sessions, a reliable ordered byte
transport, framing accumulation, wakeups and owner pumping. No listener, OS
mount, network authentication, encryption, UDP transport or Inferno namespace is
implicitly installed. [Tutorial](../examples/embedding/resource_9p/README.md),
[debugger resource](RESOURCE_DEBUGGER.md), [RU](RESOURCE_9P.ru.md).

## Wire and operations

The outer protocol follows standard **9P2000 little-endian** framing, counted
strings, qids and machine-independent stat records. Binary contents of operation
and event files retain the separate **S-record** format with canonical compact
big-endian integers, direction-local tag-zero declarations and CRC. Do not apply
9P integer packing to S payloads. This adapter is a device-server subset, not a
complete writable disk server or drop-in implementation of every Inferno feature.

| Request | Behavior |
| --- | --- |
| Tversion | NOTAG, msize from [FVM_R9P_MIN_MSIZE](../include/flashvm/r9p.h#L6) through [FVM_R9P_MSIZE](../include/flashvm/r9p.h#L5); upper values clamp. Accept 9P2000 and its dot suffixes, reply 9P2000. Unknown version replies `unknown` and requires new negotiation before attach. Reset closes fids, cancels queued file calls and discards pending reads. Quiesce owner callbacks before reset; BUSY cleanup may already have closed earlier fids and reset read tags. |
| Tauth / Tattach | Tauth returns Rerror: authentication belongs to the host transport. Attach requires NOFID afid and empty aname. uname is informational and never chooses native authority. Root is scoped to the authenticated FS session. Reject duplicate fid and NOFID; fid zero is valid. |
| Twalk | At most [FVM_R9P_WELEM](../include/flashvm/r9p.h#L8) components; zero components clones an unopened fid. Supports `..`. Partial success returns successful qids and leaves newfid unchanged; first failure returns Rerror. Open fids cannot walk. Existing different newfid conflicts. |
| Topen | OREAD for directories/path/events; ORDWR for operation files. Reject write-only, execute, truncation, remove-on-close and other flags. Iounit is min([FVM_R_BYTES](../include/flashvm/rdispatch.h#L9), msize minus the standard 24-byte I/O reserve). Use one atomic write per operation open; reopen for another command. |
| Tread | Directories contain whole stat records. Offset is zero or previous offset plus previous returned count. Rewind to zero is supported; too-small nonzero count for the next entry gives Rerror, never false EOF. Discovery changes give Rerror until reopen. PATH allows arbitrary 64-bit offsets (high half nonzero gives EOF). CALL/EVENTS have sequential Uint32 streams; no available result retains a pending read instead of returning EOF. One pending read per fid. Zero count returns an immediate empty reply. |
| Twrite | One counted raw request payload, offset zero, within [FVM_R_BYTES](../include/flashvm/rdispatch.h#L9) and negotiated whole frame. Rwrite acknowledges queue acceptance; owner pump separately executes the callback. Splitting one command across Twrites is unsupported. |
| Tflush | Suppress only an unproduced pending read by oldtag and return Rflush, including for unknown/completed tags. It does not unsubscribe, dequeue an event, cancel a prior Twrite or undo execution. Malformed flush is rejected at the native framing boundary, never answered with Rerror. |
| Tclunk | Invalidate the wire fid even on close error. Its pending read becomes a deferred Rerror unless flushed; a new fid cannot accidentally satisfy that read. A running native callback can leave retired cleanup storage until poll/destroy retries after completion. No rollback of completed effects. |
| Tstat | Standard inner stat size plus outer counted stat size. Stable qid path uses the native resource identity; qid content version, type/dev, times and file length are zero for these synthetic device files. Owner/group/muid `host` are informational. Permissions reflect supported modes; native session grants remain authoritative. |
| Others | Create, remove, wstat, authentication exchange and protocol extensions return Rerror. Files and registrations are created by trusted native APIs, not arbitrary client names. |

This device deliberately rechecks current native grants even on open files,
including buffered event fragments; it does not preserve revoked authority until
clunk. Removing then regranting SUBSCRIBE does not resurrect an old watch.
Registered resources remain host-owned; closing a connection never unregisters
shared resources or disconnects unrelated sessions.

## C API, buffers and ownership

Every call is serialized externally. The server borrows one exclusive FS; do not
use or destroy that FS independently while its descriptors belong to the server.
No managed pointers, VM roots, implicit workers, clocks or callbacks are created.
Owner execution still uses [fvm_rdispatch_pump](../include/flashvm/rdispatch.h#L145), or the debugger's existing owner
service loop. All output buffers require [FVM_R9P_MSIZE](../include/flashvm/r9p.h#L5) bytes even after a smaller
msize negotiation. Input/output and count pointers must not overlap. Retain and
send produced replies in order before later replies, including Rflush: the API
hands ownership of the produced copy to the transport, not delivery confirmation.

| Function | Contract |
| --- | --- |
| [`fvm_r9p_create`](../include/flashvm/r9p.h#L22) | Borrow FS and allocate one fixed native server; output only on OK, NOMEM preserves it. FS already carries authenticated session and native grants. One FS per connection; no socket, mount or authentication is started. |
| [`fvm_r9p_request`](../include/flashvm/r9p.h#L32) | Server, input/count, output/capacity, used/consumed outputs. Processes one complete frame, leaving a concatenated next frame unconsumed. Capacity below MSIZE gives LIMIT before mutation. Incomplete input PENDING; bad framing INVALID/LIMIT; duplicate outstanding tag CONFLICT (host terminates a violating connection). These native errors preserve output/counts. Valid request returns OK and consumed frame length; used is reply length or zero when a Tread was retained. Protocol/service errors are complete Rerror replies. Never pumps VM callbacks. |
| [`fvm_r9p_poll`](../include/flashvm/r9p.h#L40) | Server, MSIZE output buffer and used output. Checks retained reads round-robin and produces one ready reply, releasing its tag. PENDING leaves output/used unchanged. Also retries retired native closes after running callbacks finish. Read tags may complete out of order; route them by tag. Host retains produced bytes for partial-send retry. |
| [`fvm_r9p_destroy`](../include/flashvm/r9p.h#L47) | Pointer to server after transport/owner callbacks quiesce. Closes fids and discards pending reads. BUSY keeps the server pointer but cleanup may have already closed earlier fids; retry after handler completion. OK frees server and nulls pointer. Destroy FS next, optionally disconnect its session on the owner, then destroy dispatcher after all other borrowers end. |

Bounds: [FVM_R9P_FIDS](../include/flashvm/r9p.h#L7) wire fid slots and retained reads, [FVM_R9P_WELEM](../include/flashvm/r9p.h#L8) walk
components, [FVM_R9P_MSIZE](../include/flashvm/r9p.h#L5) whole frames. A retired running-close slot remains
occupied until cleanup. FS and dispatcher bounds also apply. Only creation
allocates; messages, polling and cleanup use fixed storage. Numeric offsets are
split into two Uint32 halves; C99 long long or native-endian layouts are not
required. The host accumulates a frame before calling request and bounds its
receive/transmit queues; the adapter does not keep a private transport backlog.

## Validation and platform scope

```sh
cmake -S flashVM -B build-9p -DFVM_BUILD_TESTS=ON -DFVM_BUILD_EXAMPLES=ON
cmake --build build-9p --target flashvm_srecord_oracle flashvm_r9p_debugger flashvm_r9p_lifetime flashvm_examples_embedding_resource_9p_main
ctest --test-dir build-9p -R '^flashvm_r9p_' --output-on-failure
```

`tests/resources/ninep.py` independently packs standard 9P bytes with Python
struct and builds expected S records with Python integer/zlib code. It checks
partial/in-place/clone walks, stat directory records, offsets, output atomicity,
512-byte commands, deferred reads, flush/clunk/reset, rights, limits, unknown
versions, 1500 malformed/random messages and actual IPv4 TCP loopback with
one-byte fragments plus coalesced frames. This test host supplies its transport;
it is not a built-in production C TCP listener or a mounted filesystem.
`ninep_debugger.c` runs an actual pinned AVM1 function returning 42, with two 9P
connections receiving STOP/RESUME, one authorized controller claiming/resuming
and a read-only observer querying the captured interpreter frame. It checks
source mapping and routes out-of-order replies by tag. `ninep_lifetime.c` checks
native allocation failure and clunk during a running callback, followed by
retired cleanup. None of this proves complete SWF/9P/Inferno compatibility.

The C89 API is usable from C++98. Build with the component's existing native or
cross toolchain; no new dependencies are introduced. Cross-compiling protocol
objects/fixtures does not prove a foreign OS mount, target device execution,
physical graphics/audio or full player compatibility. See
[platform validation](PLATFORM_VALIDATION.md).

Primary wire specifications: [intro](https://9fans.github.io/plan9port/man/man9/intro.html),
[walk](https://9fans.github.io/plan9port/man/man9/walk.html),
[read/write](https://9fans.github.io/plan9port/man/man9/read.html),
[stat](https://9fans.github.io/plan9port/man/man9/stat.html),
[flush](https://9fans.github.io/plan9port/man/man9/flush.html),
[clunk](https://9fans.github.io/plan9port/man/man9/clunk.html).
## v0.733 checkpoint

Linux x86-64 passed 42 selected native tests including offscreen/file/memory/HTTP
player paths, plus 7 shared-host tests with HTTP disabled. New C modules,
lifecycle, tutorial and real AVM1/9P fixture passed ASAN/UBSAN; the underlying VM
archives in that sanitizer fixture were ordinary builds and leak enumeration was
disabled. This is not a full sanitized VM acceptance run. The C89 9P tutorial
cross-linked for ARM64 Linux musl, RISC-V64 Linux musl and Windows x64 GNU;
debugger fixture and C++98 public ABI probe compiled as target objects. Foreign
binaries were not executed. SDL checks used dummy/offscreen, not physical devices.

Use an explicit toolchain from the repository root (replace the absolute path):

```sh
cmake -S flashVM -B build-cross-9p \
  -DCMAKE_TOOLCHAIN_FILE=/absolute/path/to/target-toolchain.cmake \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/minimal.defs" \
  -DFVM_BUILD_TESTS=OFF -DFVM_BUILD_EXAMPLES=ON
cmake --build build-cross-9p --target flashvm_srecord flashvm_examples_embedding_resource_9p_main
```

The toolchain selects OS, CPU, compiler and sysroot; copy the target executable
to that platform for a separate execution check. In a detached component use
`-S .` and `$PWD/cmake/profiles/minimal.defs`. NDK/MinGW preparation and native
acceptance are covered in [platform validation](PLATFORM_VALIDATION.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_r9p_create` | function | [include/flashvm/r9p.h:22](../include/flashvm/r9p.h#L22) |
| `fvm_r9p_destroy` | function | [include/flashvm/r9p.h:47](../include/flashvm/r9p.h#L47) |
| `FVM_R9P_FIDS` | macro / define | [include/flashvm/r9p.h:7](../include/flashvm/r9p.h#L7) |
| `FVM_R9P_MIN_MSIZE` | macro / define | [include/flashvm/r9p.h:6](../include/flashvm/r9p.h#L6) |
| `FVM_R9P_MSIZE` | macro / define | [include/flashvm/r9p.h:5](../include/flashvm/r9p.h#L5) |
| `fvm_r9p_poll` | function | [include/flashvm/r9p.h:40](../include/flashvm/r9p.h#L40) |
| `fvm_r9p_request` | function | [include/flashvm/r9p.h:32](../include/flashvm/r9p.h#L32) |
| `FVM_R9P_WELEM` | macro / define | [include/flashvm/r9p.h:8](../include/flashvm/r9p.h#L8) |
| `FVM_R_BYTES` | macro / define | [include/flashvm/rdispatch.h:9](../include/flashvm/rdispatch.h#L9) |
| `fvm_rdispatch_pump` | function | [include/flashvm/rdispatch.h:145](../include/flashvm/rdispatch.h#L145) |
<!-- source-links:end -->
