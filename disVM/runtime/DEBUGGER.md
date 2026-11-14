# Retained continuation debugger

DisVM contains its own native supervisor and bounded command protocol. Running
and inspecting it requires no installed GDB, Python interpreter, debugger library
or external service. Python programs in `tools/` and the examples are optional
host clients, not runtime dependencies. OS threads and system calls remain host
facilities. The execution profile remains dedicated native threads, exclusive=0.

Enable explicitly before starting guests:

```
disvm --debug /private/new-endpoint --jit application.dis
disvm --debug-fds 3 4 --jit application.dis
disvm --gdb --jit application.dis
```

`--debug` creates a new mode0600 Unix socket, refuses an existing path and cleans
up only its own inode. `--debug-fds` duplicates separate inherited input/output
FDs>=3. Neither uses guest stdin/stdout. The private-pipe transport is also useful
on hosts that deny sockets. Protect the containing directory and inherited FDs.
Both native transports use the same protocol; no client package is required.

`--gdb` must be the first flag. Only this explicitly selected launcher invokes
an installed `gdb` through PATH. It requests non-stop mode before loading the
runtime, forwards other VM flags and opens the GDB prompt. GDB target support
controls its actual stopping behavior; check it before relying on non-stop.
It can be combined with `--debug`. Native symbols depend on the build's debug
information. A missing GDB is an explicit launch error, not a runtime dependency.

## Commands and ownership

Start with `hello 0`. It returns `session` (32 random hex characters) and host PID.
Prefix every subsequent line with `@SESSION `; old sessions cannot control a
replacement VM through a reused endpoint/PID. One line is at most255 bytes,
response at most4095 bytes. Commands are synchronous transport requests but
pause/terminate acceptance is asynchronous: poll status for acknowledgement.

| Command | Result or effect |
| --- | --- |
| `list AFTERPID` | Next resident with larger PID; iterate the live registry |
| `status PID` | Copied location/opcode/error and pause acknowledgement |
| `registers PID` | VM REG fields; running values are last publication |
| `operation PID` | Original Dis opcode and encoded operands |
| `pause PID` | Request a source-owned boundary/native reentry stop |
| `continue PID` | Release retained source continuation; original handler runs |
| `terminate PID` | Source applies termination and existing native cancellation |
| `step PID` | One original Dis instruction from a boundary pause only |
| `backtrace PID INDEX` | One bounded frozen Dis frame, index<128 |
| `read PID stack\|mp FRAME OFFSET COUNT` | Up to64 bytes in a typed frame/MP extent |
| `object PID stack\|mp FRAME SLOT OFFSET COUNT` | Read object reached by a mapped strong-reference slot |
| `census PID` | On-demand deduplicated graph census from this source's published GC roots |

Memory reads require `retained:true`, use ordinary shared range guards and
cannot accept arbitrary native addresses. Register pointer values are diagnostic
numbers, not address-read capabilities. No getter or guest callback is invoked.
The inspector has its own GC admission; it never executes another owner's Prog.

A spinning Dis loop stops at the next checked instruction boundary. A blocked
channel owner publishes its own TLS roots. A source in an audited native wait
publishes roots before leaving managed admission; the debug gate prevents its
managed reentry. This does not suspend an arbitrary OS syscall halfway through
its host side effects. Resume keeps the original C continuation/TLS registers;
REG snapshots are never used to restore or migrate the source.

In debug mode a managed exception stops before `handler()` destroys frames or
exception state, including first-chance caught exceptions. The original stack,
process roots and exception remain retained until explicit continue/terminate.
Other guests and GC continue. Continue delivers the existing handler; continuing
an unhandled exception retires that source and makes final application status
nonzero without killing unrelated guests. Terminate is an explicit debug action.
An unreleasable continuation reports `fault_stop_refused`, not a false freeze.
Step refuses fault/native/idle stops: it never replays an uncertain operation.

Disconnect preserves held targets. Pipe EOF does not resume them; socket clients
can reattach. Pipe response timeout/truncation means unknown outcome, not success;
query status through a usable connection before another control action. Debug
shutdown wakes gates and joins the inspector before GC shutdown, then frees cards
only after source threads join. An abandoned pipe session may require an OS-level
process termination if no other controller owns its descriptors.

## Census scope

Census traverses published stack extents, module roots, exval and mapped strong
edges using query-local native storage; no permanent per-object accounting table
is added. Aliases/cycles are deduplicated while incoming guards stay held.
Counts are reachable objects and allocator capacity bytes (including header/slack),
not requested payload/RSS or exclusive per-thread ownership. Shared graphs must
not be summed across thread reports. Unrelated process roots, native queues,
off-arena allocations and host resources are outside this one-source graph.

Bounds:256 objects,384 fields,4096 map bytes,128 frames,64 extents. Status bits:
complete1,limit2,busy4,invalid8,opaque16. A limit preserves only a partial prefix;
busy/invalid zero the counts. Opaque edges cannot establish completeness. These
results are a debug observation, not a GC marking/retention operation.

## Native JIT failures and original Inferno

Original Inferno exposes process control/stack debugging through `emu/port/devprog.c`:
`dbgxec`, `/prog` controls and `progstack`; `pctab` maps native PCs back to Dis PCs.
Linux `emu/Linux/os.c` translates SIGILL/SIGBUS/SIGSEGV/SIGFPE through `disfault`.
This references the pinned source design, not permission to import its original
single-executor assumptions into the native multi-thread runtime.

See [JIT quarantine](JIT-QUARANTINE.md) for whole-function fallback and logs.
Current built-in retention covers managed throws and audited native waits.
It does **not** catch arbitrary OS native faults or promise safe continuation of
corrupted RX execution. GDB may inspect those faults independently when installed;
the built-in protocol must not depend on it. Internal signal-context capture and
classification/parking of such faults remain work, including async-signal safety,
held guards, owner admission, register mapping and partially completed writes.
