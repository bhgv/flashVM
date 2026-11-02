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
release sites. Ordinary FILE-backed operations keep the heap gate because their
cursor/error state needs external serialization. Compiler callbacks and arbitrary
C extensions do not automatically become resumable. A blocked OS operation can
delay shutdown: no unsafe forced cancellation is attempted.

The example uses the existing private native-cell module ABI, not a newly
accepted shared Dis64 file-call profile. Verification here is native Linux x64;
this does not claim ARM/RV execution of the new pool.
