# On-demand namespace inspection

The independent native debugger can inspect the namespace of a retained guest.
Use `pause PID`, wait until `status PID` reports retained=true, or inspect a
retained first-chance fault. The guest's original continuation, namespace group
and resources remain alive. Queries neither resume the guest nor execute guest
code. Other native owners continue; a shared group can still change.

Commands use the existing private socket/pipe session envelope; ordinary stdin,
stdout and guest channels carry no debugger traffic. No installed debugger or
library is required by the VM. The Python driver is a development client only.

| Command | Result |
| --- | --- |
| `namespace PID` | Current revision, binding count, group reference count and root/cwd/import held-handle flags. |
| `namespace-read PID REVISION INDEX FIELD OFFSET` | Up to128 bytes from one text field at the exact expected mutation revision, encoded as hex. |

Revision is returned as a decimal JSON string to preserve all64 bits in clients.
Read responses include full byte length, copied count and selected binding-held
flag. Offset equal to length succeeds with zero copied bytes. Larger offsets,
wrong indexes, unsupported fields, integer overflow, signs and trailing tokens
are refused. Output fits the existing4096-byte debugger response bound. Text is
not silently truncated: request further offsets until full length is reached.
Chunks are raw UTF-8 bytes; decode only after joining them, since a chunk may
split a Unicode character. Hex also preserves quotes/control characters exactly.

| FIELD | Text | INDEX |
| --- | --- | --- |
|0| Captured native root display spelling |0|
|1| Virtual cwd |0|
|2| Virtual import directory |0|
|3| Captured display source of held cwd |0|
|4| Captured display source of held import directory |0|
|5| Binding virtual target |0 .. bindings-1|
|6| Binding captured native source |same binding index|
|7| Binding component: empty for directory, basename for regular file |same binding index|

Binding order belongs only to that group's current revision; FORKNS need not
preserve list order. Find a target by reading field5 at each index. Any mutation
invalidates the entire old enumeration, even if the selected text did not change.
`namespace changed` means discard collected pages and restart from summary.
Summary and every slice copy under the group mutex. There is no persistent
snapshot allocation, per-object statistics table, filesystem I/O or GC traversal.
Counting bindings and selecting an index walks existing records under that mutex;
this is not a lock-free or constant-time query. References count group owners at
that instant (including startup controller); spawn/retirement can change them
without a namespace mutation revision. They are not per-thread ownership or
memory-byte accounting.

Only retained targets are accepted; a running target returns `not retained`.
The debugger pins the source through its existing wake/native gate, then reads
only native namespace state. It does not enter a heap inspector or dereference
client-supplied addresses. Shared peers may mutate between requests, which is why
revision checking is required. There is no global execution exclusion.

Display sources can be stale after host rename. Held flags show native capability
ownership, not proof that its display name is currently reachable or that a child
file is immutable. Credentials, FD groups, complete Chan/provider graphs, union
mounts and provider-specific diagnostics remain future work. Non-Linux debugger
transports/runtime adapters remain unported; mandatory architecture CI includes
this workload but local executable evidence is Linux x64.

[Public native observation contract](vfs/observation.h),
[implementation](vfs/observation.c), [protocol](debugger/namespace.c),
[executable example](../examples/namespace-debug/README.md),
[Russian](NAMESPACE-DEBUG.ru.md).

Validation:13 focused ordinary and14 focused TSan CTest groups passed, including
namespace, imports, directory anchors, retained debugging, mailbox, owner stack/
location, memory statistics, GC requests, errors and load cancellation. The new
driver includes two actual mixed-revision omission controls in both builds.
Native core (including concurrent observations) passed ASan/UBSan with
ASAN_OPTIONS=detect_leaks=0; no LSan or whole-runtime sanitizer claim. New source
files are below500 lines; local documentation targets resolve.
