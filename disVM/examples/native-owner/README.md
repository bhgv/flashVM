# Native work with per-owner call state

This public C API tutorial consists of separate files:

- [native_wait.h](native_wait.h): borrowed arguments, callback/error contracts.
- [native_wait.c](native_wait.c): active → native → active round trip; even a
  failing native operation reacquires the heap before publishing its scalar.
- [main.c](main.c): standalone bind/register/enter/seal/leave/detach/unbind example.

From the installed disvm-editor or disvm-runtime SDK root on Linux:

```sh
cc -std=c99 -Wall -Wextra -Werror -I include \
  examples/native-owner/main.c examples/native-owner/native_wait.c \
  -L lib/host -ldisvm_flashvm_host -ldisvm_shared_ports -pthread \
  -o native-owner-example
./native-owner-example
```

Expected output includes `status=-7`, `matched reentry`, `simulated_epoch=1` and
`retired`. This single-owner demo deliberately simulates an epoch and failing
native status. It does not execute Dis or claim a real collection. The runtime
and the multi-owner fixture supply actual GC/root/OS-owner evidence separately.
Protocol/allocation errors terminate this standalone demo process; the reusable
helper returns zero only before leaving ownership and aborts on broken reentry.
A callback must return a status on ordinary I/O failure, never throw or cancel.

The production host stores DVM_HeapOwner inside DVM_NativeContext, reached through
the existing TLS key. No second TLS owner, managed allocation or process-global
native-call flag is introduced. The opaque metadata holds no managed references.
Only its own OS thread may inspect/mutate it; foreign pointers are rejected before
any private counter read. Shared heap/collector state is not copied per thread.

| Phase | Allowed next operations |
| --- | --- |
| DETACHED | attach to a live serialized world |
| IDLE | enter the FIFO turn, or detach immutable roots |
| ACTIVE | leave, audited suspend, or seal when the last registered owner |
| NATIVE | resume on the same OS owner |

After resume, ACTIVE has an outstanding epoch sample: call progress exactly once
before another transition. Crucially, read the shared collector epoch **after**
resume, not while waiting for admission. The helper demonstrates both placement
and failure-status handling. Unsigned epoch wrap is tested independently.

Before suspending a real VM callback, publish its complete REG/managed frame and
register all otherwise hidden managed C-stack owners. Only immutable native
snapshots/handles may reach the operation callback. Inactive published roots
stay unchanged until entry/detach. Metadata is not a transferable native token or
channel payload. Attaching alone neither creates a Prog nor makes Sys/interpreter
entry safe; a host must provide complete process/root/scheduler ownership.

Sealing atomically closes *registration* only for the last active owner. If a
peer remains, sealing fails without closing anything. The world/service must
remain alive through every attach attempt and native return. Hosts publish the
service before starting workers and join all attempts/owners before destroying
it. Sealing does not make a borrowed raw world pointer safe after destruction,
cancel queued tickets or join OS threads for the caller.

The executable [native_owner fixture](https://github.com/bhgv/flashVM/blob/master/disVM/tests/native_owner.c) uses two real
OS owners, retained addw and actual stopped-root scans. Its driver also links
this exact native_wait.c, checks negative status and wrapping epochs, and runs
active removals of FIFO exclusion, epoch capture and registration sealing:

```sh
python3 disVM/tests/native_owner.py --flashvm-root flashVM
CFLAGS='-O1 -g -fsanitize=undefined -fno-sanitize-recover=all' \
  python3 disVM/tests/native_owner.py --flashvm-root flashVM
CFLAGS='-O1 -g -fsanitize=thread' \
  python3 disVM/tests/native_owner.py --flashvm-root flashVM
```

Run these fixture commands from a source checkout, not the installed SDK. Each
positive run checks 201 snapshots of two roots, independent overlapping native
waits, FIFO reentry, private counters and native retirement. The focused TSan run
passes without race reports; executable-name lookup warnings limit symbolization.
It is not whole-managed-VM TSan coverage. Parallel managed spawn and concurrent
marking remain open.
