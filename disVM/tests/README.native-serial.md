# FIFO heap admission before managed parallel scheduling

`native_serial.c` is a native ownership fixture, not a miniature replacement Dis
VM. Its arithmetic body is extracted byte-for-byte from retained `addw`. Two
actual OS threads own separate extended `up`/`REG` contexts and share a plain
non-atomic operand. The production world gate is their only arithmetic ownership
mechanism. A third context on main performs stopped-root scans.

Run from the repository root:

```sh
python3 disVM/tests/native_serial.py --flashvm-root flashVM
CFLAGS='-O1 -g -fsanitize=undefined -fno-sanitize-recover=all' \
  python3 disVM/tests/native_serial.py --flashvm-root flashVM
CFLAGS='-O1 -g -DDVM_WORLD_NO_ATOMICS=1' \
  python3 disVM/tests/native_serial.py --flashvm-root flashVM
```

Every invocation builds a private temporary executable and runs atomic-request
and locked polling. The fallback build always uses locked request observation.
A deterministic handshake holds owner zero's initial heap turn until owner one
has queued. The older waiter must enter before owner zero can reenter. Owner
zero then waits in native code with published immutable registers while owner
one executes at least 16384 more retained instructions. No sleep duration decides
correctness; a subprocess timeout only bounds a broken protocol.

Main makes 100 stopped-root scans during that native wait. Both workers then
perform 10000 short enter/instruction/leave turns each while main makes another
100 scans. A final scan runs after both owners finish but before detach. Every
scan sees exactly two root sets and at most one active mutator, with coherent
register pointers and mirrored results. Exact total arithmetic, drained FIFO,
foreign/nested-entry refusals, live-world destruction refusal and all TLS/world
retirements are checked.

The driver also compiles a deliberate removal of the actual exclusive-active
predicate. It must fail immediately with the expected FIFO violation; a timeout,
missing executable or arbitrary failure is not accepted as a successful control.
It does not mutate any repository source.

The public native API is in `include/disvm/native_world.h`. Construct a serialized
world with `dvm_world_create_serial(locked_poll)`, join each bound native owner,
then pair enter/leave. An inactive owner must keep all published process/register
storage immutable and alive until reentry or detach. An owner waiting for its
FIFO ticket cannot be cancelled. Admission does not discover arbitrary C-stack
roots and does not make an unsynchronized scheduler safe. Diagnostics from
`dvm_world_admission` are snapshots, not ownership reservations.

The production GC-worker runtime now uses this constructor. Its sole existing
Dis executor takes the initial turn and releases/reenters through audited native
I/O/wait boundaries. `NATIVEGC` reports serial_heap=1, admissions, contentions and
waiters=0 at shutdown. The normal instruction fast-poll still takes no mutex when
there is no collection request; the FIFO mutex is used at turn boundaries.

Managed parallel spawn remains disabled. Runnable/process lists, current-process
assumptions, per-owner native-call state, module caches, channels and RC mutation
need to obey this gate or obtain separate proven synchronization before a second
managed executor is started. Concurrent marking is a separate graph-barrier task.

The same native fixture was also run with CFLAGS='-O1 -g -fsanitize=thread':
both polling routes and the active removal control completed without a data-race
report. The runtime warned that executable-name lookup failed, limiting stack
symbolization. This is focused native gate evidence, not TSan coverage of the
whole managed VM.
