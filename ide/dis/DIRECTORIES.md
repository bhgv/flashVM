# Channel-owned directory browsing — Idefiles

[Русский](DIRECTORIES.ru.md) · [Interface](bridge/idefiles.m) ·
[Implementation](bridge/idefiles.b) · [Executable tutorial](examples/directory-browser/README.md) ·
[OS ports](../../disVM/runtime/FILESYSTEM.md)

The IDE now uses a real Limbo directory owner over Sys stat/fstat/dirread.
[`Idefiles.startdirs(sys)`](bridge/idefiles.m#L82) returns a [`chan of DirectoryRequest`](bridge/idefiles.m#L56); nil Sys returns
nil. This endpoint is separate from `start(sys)` byte windows, preserving their
existing protocol. All native filesystem access stays inside selected OS ports.
There is no native IDE policy implementation or imported Inferno GUI dependency.

The console IDE exposes `dir PATH`, `dirnext`, `dirclose`, and `stat PATH`.
`dir` opens and displays one batch; `dirnext` advances once, so a huge directory
does not automatically flood the terminal. The new cursor replaces the previous
one only after successful open. Failed replacement preserves the old position.
These commands leave text, revision, dirty state and history unchanged. Names
are shown in full, in bounded chunks; C0/C1 controls and quote/backslash characters
are escaped before presentation. Native names and returned records stay unchanged.

```text
dir /path/to/project
dirnext
stat /path/to/project/main.b
dirclose
```

This is an implemented console frontend and a reusable channel interface for the
FlashVM frontend. A graphical file-tree widget is still separate work.

## Protocol and ownership

Include matching `sys.m` and `idefiles.m`. Load the rebuilt `idefiles.dis` beside
the application. Keep request values immutable until the private reply is drained.
Every admitted request sends exactly one correlated reply. A nil reply is ignored
before any state change; negative request IDs return Invalid. Normal exchanges
produce no unsolicited diagnostic events.

| Operation | Input | Result and lifecycle |
| --- | --- | --- |
| OpenDir | cursor=0, valid path | Ok with a positive `big` cursor and fstat metadata of the actual opened directory; no batch yet. |
| NextDir | live positive cursor | Ok with 1–32 independent records; End with nil at EOF. |
| CloseDir | live positive cursor | Close the native FD and retire identity before Ok; duplicate close is Stale. |
| StatPath | cursor=0, valid path | Ok with copied metadata; consumes no directory slot. |
| Shutdown | nonnegative request ID | Close all native FDs before acknowledging Ok, then terminate this owner. |

The owner holds at most eight cursor identities. Full means quota or monotonic
identity space exhausted. IDs increase without reuse; closed/failed IDs cannot
select a later cursor occupying the same array slot. IDs are **endpoint-local**:
carry `(endpoint, cursor)` together. A bare number from another owner can coincide
with a live local number; this is not a globally unique or authenticated network
capability. Stale means absent/retired in the receiving endpoint.

The native FD is released at EOF. The explicit identity remains until CloseDir
or Shutdown; repeated NextDir returns stable End without another native call.
An errored native batch publishes no partial entries, retires its identity and
returns Failed; following requests with that ID return Stale. The owner never
keeps a result array as cursor state, so callers may mutate received arrays.
Metadata and entries survive cursor closure. No raw FD/native pointer is exported.

Paths must contain valid Unicode scalars, no NUL, 1–4096 UTF-8 bytes. Scalar
length is bounded before UTF-8 conversion allocates memory. OpenDir/StatPath
require cursor=0; NextDir/CloseDir ignore path. Unknown operations return Invalid.
Ordinary files cannot become directory cursors. Android `content://` remains
unsupported at the OS port. Native failures carry a failure-only plain message.

Private scalar state arrays are explicitly initialized. Limbo ordinary scalar
array allocation leaves their contents unspecified; using implicit zeros can
reintroduce stale slot state after allocator reuse. Reference arrays use the VM's
normal nil initialization. Cursor close clears owner references before replying.

## Scheduling and limits

Native I/O still blocks the single VM owner. Distinct Dis processes/endpoints do
not make Sys calls asynchronous. A caller that fails to drain its reply blocks
that service; there is no abandoned-reply cancellation or network lease here.
Enumeration is not an atomic snapshot; order is native, not globally sorted.
External changes, invalid UTF-8 filenames and dangling links may fail a batch.
Use the existing Readdir library when an explicitly bounded complete sorted
listing is appropriate; this streaming owner does not accumulate the directory.

Runtime 0.27.0 also fixes explicit execution quotas: `--max-instructions COUNT`
now supersedes the legacy scheduler-slice budget. Quantum 1 can therefore execute
more than 100000 instructions when explicitly allowed. Without that option, the
existing busy-work slice policy and input/UI progress reset remain unchanged.
The explicit quota remains global, exact, independent of quantum, and unreset by
UI/stdin progress. It is not a wall-clock timeout or cancellation of blocked I/O.

## Verification

The executable tutorial uses real files: Unicode, embedded binary NUL, a sparse
4294967303-byte file and 74 directory entries. It checks three batches, array
independence, metadata retention, empty/error directories, EOF, quota, invalid
requests, retired-ID rejection, repeated close/open and 32 terminal owner cycles.
Both modes run at quanta 1/2048. Console replay additionally checks document
preservation, failed replacement, control escaping and shutdown with an open cursor.
The client exceeds 100000 instructions and thus covers the quota-policy fix.
Native target acceptance is Linux AMD64; other OS ports retain their documented
platform acceptance limits. No `/prog` debugger, distributed object protocol,
object mutex or new compiler syntax is claimed by this service.
