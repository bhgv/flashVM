# Two managed OS executors

[Runtime protocol](../../runtime/THREADING.md#two-managed-os-executors-0520) ·
[Русский](../../runtime/THREADING.ru.md#два-os-исполнителя-0520) ·
[Release](../../../docs/releases/v0.1748.md)

These are real Limbo programs compiled by the bundled compiler and executed by
our DisVM. `spawn` creates logical Dis processes; the runtime assigns their
managed turns to one to eight OS executors. The default remains one executor.
There is one shared managed heap. Its writes remain serialized by NativeWorld;
audited native waits can release it so another executor makes progress.

Build/install `disvm-runtime` or `disvm-editor` through `build-menu.py`.
Entry **19** runs the installed `run-managed-workers.sh`. Alternatively:

```sh
# Console example; no graphical server or external Inferno emu required.
sh out/disvm-editor/run-managed-workers.sh --no-jit --quantum 31

# Run the same module through JIT. Both paths use the same scheduler protocol.
sh out/disvm-editor/run-managed-workers.sh --quantum 31
```

The launcher enables `--gc-worker --managed-workers 2 --stats`. Explicit
`--managed-workers 1` is the reference configuration when invoking `disvm`
directly; do not add it to this launcher, which already selects the count.

## Programs and observable results

| Source | Installed module | What it exercises |
| --- | --- | --- |
| `main.b` | `managed-workers.dis` | Parent and two actors compute deterministic sums, exchange channel messages, retain a cyclic graph, catch errors and preserve per-process `%r`/FP rounding across migration |
| `backpressure.b` | `managed-backpressure.dis` | Two actors each write100003 bytes to standard output; the test holds a real pipe full until both native calls are suspended |
| `shared-file.b` | `managed-shared-file.dis` | Two actors share one ordinary file descriptor;128 writes and128 formatted writes preserve counts and cursor state |
| `ui-rpc.b` | `managed-ui-rpc.dis` | The graphical host accepts copied UI requests from a claimed helper turn while SDL/FlashVM remain on the UI owner |

Every source documents its arguments, shared state and expected checks. Main
finishes with `PASS managed workers`; runtime then prints ordinary completion.
`NATIVEPOOL` must show `joined=1 failed=0` and positive controller/worker turns.
`NATIVEDISPATCH` reports balanced claims/finishes, `active=0`, and migrations.
Counts vary with scheduling; instruction totals for the same module/mode do not.

The backpressure module writes binary payload before its PASS marker: use the
driver rather than an interactive terminal. The shared-file module creates
`managed-shared.bin` in its working directory; the driver gives it a private
temporary directory. The UI module needs `disvm-editor` and an explicit font;
the driver supplies the packaged font and dummy SDL environment for headless CI.

## Larger bounded pool

Runtime0.53.0 accepts `--managed-workers 1..8`; counts above1 still require
`--gc-worker`. The installed launcher defaults to2. To choose eight:

```sh
DVM_MANAGED_WORKERS=8 sh out/disvm-editor/run-managed-workers.sh --quantum 31
```

The new `backpressure-eight.b` module (`managed-backpressure-eight.dis`) starts
eight actors, each retaining a cyclic root and writing100003 bytes. The capacity
driver holds the actual OS pipe full until all EIGHT native continuations are
suspended, then counts800024 bytes. Run it inside the installed editor package:

```sh
python3 source/threading/test_managed_pool.py \
  ./disvm-editor ./managed-workers.dis ./managed-backpressure-eight.dis \
  ./limbo-native ./include/limbo ./fonts/NotoSansMono-Regular.ttf
```

For4/8 compute runs, each `NATIVEPOOL_WORKER` record must show real bytecode
turns, balanced releases/reentries and a completed join. These are native lifetime
and conservation checks, not a promise that serial heap computation gets faster.
The console driver also compiles `test_managed_start.c`, a Linux-only private
pthread interposer. It refuses creation after0/1/5/6 helpers and requires every
successful thread creation (including the two collectors) to have a successful
join. A non-hit ordinal checks normal forwarding. This facility is test-only;
production has no thread-failure injection option. The shared library is loaded
from the private child working directory, so paths containing spaces remain
covered without passing a whitespace-split absolute `LD_PRELOAD` path.
Do not run the backpressure payload module directly into an interactive terminal.

## Reproduce the complete admission test

From an installed `disvm-editor` directory:

```sh
python3 source/threading/test_managed_workers.py \
  ./disvm-editor ./managed-workers.dis ./managed-backpressure.dis \
  ./managed-shared-file.dis ./managed-ui-rpc.dis \
  ./limbo-native ./include/limbo ./fonts/NotoSansMono-Regular.ttf
```

The driver compares against a one-executor reference, tests interpreter and JIT,
quanta1/31/2000 and locked/atomic collector polling. It independently counts
200006 pipe bytes, waits for actual two-call suspension rather than assuming
timing, verifies the shared-file contents and helper UI admissions, then checks
unhandled exceptions and a global instruction quota. Error exits must also join
the helper and collectors and retire every managed claim.

## Current boundary

Two executors require `--gc-worker`; the graphical host also requires its default
threaded UI. This is cooperative scheduling around one exclusive heap, not
simultaneous managed heap writers. Standard-stream I/O and sleep are audited
release sites. Ordinary read/write/fprint/seek use a per-file FIFO and can release the heap.
Opening/closing and private loader callbacks retain their synchronous
boundary; metadata and directory batches can release the heap; zero-length write validation also stays on the active owner. Compiler callbacks and arbitrary
C extensions do not automatically become resumable. A blocked OS operation can
delay shutdown: no unsafe forced cancellation is attempted.

The example uses the existing private native-cell module ABI, not a newly
accepted shared Dis64 file-call profile. Verification here is native Linux x64;
this does not claim ARM/RV execution of the new pool.


## Ordinary files outside the heap (runtime0.54.0)

`shared-file.b` now also tests relative seek after writes, rewind/readback and a
refused negative seek. Native lease counters must include ordinary file calls;
`NATIVEFILES` requires no pending gates and matched gate creation/retirement.
The historical `serialized_files` counter now means per-file FIFO admissions,
not retention of the global heap gate.

`file-overlap.b` (`managed-file-overlap.dis`) has two writers sharing ONE FD and
an unrelated computing actor. The test-only `fwrite` interposer blocks the first
ordinary file write in a real OS pipe read. The second writer waits on the file
FIFO; the parent must compute/print BEFORE the driver releases the native barrier.
This is a controlled blocking test, not a benchmark or measured slow disk.
After release, the driver checks both file bytes, FIFO contention, paired native
leases and complete gate retirement at4/8 executors in interpreter/JIT.

Inside an installed editor package:

```sh
python3 source/threading/test_native_file_wait.py \
  ./disvm-editor ./managed-file-overlap.dis ./fonts/NotoSansMono-Regular.ttf
```

From a full source checkout, `python3 disVM/runtime/test_file_gate.py` compiles
the actual native FIFO and OS-context adapters. It reverse-starts eight ticketed
workers and checks foreign/duplicate completion and busy/reserved retirement.
Three private removal controls must fail semantically. The default uses UBSan;
`CFLAGS='-O1 -g -fsanitize=thread'` selects the focused native TSan check.
This is not whole-VM race-freedom proof.


## Metadata and directory waits (runtime0.55.0)

`metadata-overlap.b` (`managed-metadata-overlap.dis`) tests four actual native
wait sites: UTF-8 path stat, open-file fstat, open-directory fstat and directory
batch read. The private Linux/glibc interposer holds stat64/fstat64/readdir64 in
an OS-pipe barrier. Only AFTER observing each native pause does the driver send
a stdin token. An unrelated actor must then convert another UTF-8 path, compute
and print BEFORE the metadata barrier is released. No elapsed sleep establishes
success. These are controlled waits, not measured slow-filesystem performance.

The example also starts two directory enumerators sharing one FD and verifies
all65 names exactly once across the32-entry native batch boundary. VM records
and strings are built only after managed reentry; native batches are freed on
success/EOF and protected publication failure. The directory FIFO also covers
abort and close. FIFO orders native operations, not multi-call transactions or
whole-directory snapshots; completed concurrent batches cannot be rolled back.

Inside an installed editor package (requires a C compiler and Linux/glibc for
the test-only interposer):

```sh
python3 source/threading/test_metadata_wait.py \
  ./disvm-editor ./managed-metadata-overlap.dis ./fonts/NotoSansMono-Regular.ttf

# Opt into the same pool in the actual graphical IDE; default launch is unchanged.
sh ./run-desktop-ide.sh --gc-worker --managed-workers 4
```

`NATIVEMETADATA` reports path/descriptor attempts, actual native lease releases,
FD pins, native PATH staging bytes/peak and directory read attempts (`batches`).
The native port owns directory-batch allocation separately; path staging is not
a whole-process native-memory accounting claim. Normal retirement requires pins0,
staging0 and no pending descriptor gates. This new test checks both interpreter
and JIT with4/8 executors, two simultaneous UTF-8 snapshots and complete gate
retirement. Open/create/close, private loader work and protected native64holds/
gchalt regions retain their prior synchronous policy; forced OS cancellation is
not added. The stable default remains one executor, with opt-in worker GC.


## Elastic pool (runtime0.56.0, current policy)

The historical1..8 restriction above no longer applies. `run-managed-workers.sh`
now defaults to `DVM_MANAGED_WORKERS=auto`; set a positive integer for a fixed
count. The runtime itself still defaults to one. Automatic mode grows for audited
blocking I/O and retires idle extra OS threads; parked logical channel tasks have
no dedicated OS thread. `--managed-workers-limit N` optionally bounds live auto
executors; resource refusal is fatal, not graceful cancellation.

`elastic.b` parks256 receivers and64 constant senders, then runs two waves of12
writers against real OS pipe backpressure. The driver holds the pipe until native
suspension is observed, drains exact payloads, waits for actual idle joins and
releases stdin for the next wave. This tests context reuse and regrowth rather
than assuming elapsed time proves blocking. `elastic-stop.b` holds four native
readers until a managed failure is observed, then releases them to test stop/join.
`elastic-park.b` deliberately leaves256 tasks parked after init returns; its private
driver terminates that nonterminating fixture and does not claim managed cancellation.

From an installed product, run (replace ROOT with its absolute directory):

```sh
python3 ROOT/source/threading/test_elastic_pool.py \
    ROOT/disvm ROOT/managed-workers.dis ROOT/managed-elastic.dis ROOT/fonts/NotoSansMono-Regular.ttf
```

For the graphical host use `ROOT/disvm-editor` and its installed font. The test
selects interpreter and JIT itself. The Linux driver uses private pipe capacity
controls; it is not a portable platform acceptance test. See
[release](../../../docs/releases/v0.1752.md) and
[remaining work](../../runtime/MULTITHREADING-ROADMAP.ru.md).


## Open/create overlap (runtime0.57.0)

`metadata-overlap.b` now adds file-open, directory-open and exclusive-create
phases. During each held native operation the parent creates a different UTF-8
file, converts another path and computes/prints. Thus an unpublished descriptor
reservation must survive a competing open, and string2c scratch reuse must not
alter the held path. Missing-file and existing exclusive-create attempts verify
failure publication/retirement. The driver interposes fopen64/opendir AND open64:
exclusive creation uses the latter rather than fopen64. Native barriers are test
controls only, with no runtime/environment dependency added to the adapter.

Run `source/threading/test_metadata_wait.py` with the runtime executable,
`managed-metadata-overlap.dis` and font path. The driver selects C/JIT and4/8
executors. All seven phases require observed managed progress before OS permits
are sent; elapsed delays alone do not count as overlap evidence.


## Native retirement (runtime0.58.0)

The metadata-overlap example now drops the last references to a file and directory
in phases7/8. Private fclose/closedir barriers pause native retirement after the
mapped FD has detached its handles. Parent progress requires creating a competing
file and computing/printing before either close is allowed to finish. Thus moving
GC to another thread alone cannot accidentally satisfy this test while retaining
the common heap during close. The final NATIVECLOSE record must have matching
submitted/completed, pending0 and joined1. The same driver now runs nine phases.

Prepared close jobs avoid allocation in finalizers. Physical native handles may
outlive released descriptor slots until the close worker consumes them. Shutdown
waits for this queue; the fixture is not a forced cancellation test.


## Helper RC admission (runtime0.59.0)

`rc-overlap.b` runs32 producers replacing arrays of sixteen node references over
sixteen rounds. Each producer checksum is3840; total122880. Audited native sleeps
distribute continuations across OS owners. The external driver withholds stdin
until aggregation prints completion; this is a logical parent-process wait, not
an assumption that the original OS controller cannot run other Progs.

`source/threading/test_helper_rc.py` takes executable, managed-rc-overlap.dis and
font. It selects C/JIT and4/8/auto, requires positive helper_enqueued and exact
final RC balance/zero bytes/worker joins. Role-only test_rc_roles.py separately
uses real world transitions and three active removal controls. Its forbidden
payload sentinels do not replace the whole-program allocation/GC checks.
