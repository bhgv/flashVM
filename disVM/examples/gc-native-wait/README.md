# Audited native wait outside the managed heap interval

[main.b](main.b) holds a cyclic Node, array, string and four nested managed frames
across four positive Sys.sleep calls. Local array sentinels and graph/string data
are checked after every return. Negative/zero periods return zero. A native-cell
wide value constructed by shifting a runtime array length is rejected with -1
when it exceeds the native32 OS period range. This uses the matching NC64 SDK,
not the restricted shared Dis64 scalar/file-call profile.

From the installed disvm-editor package:

```
./limbo-native -g -I include/limbo -o native-wait.dis examples/gc-native-wait/main.b
./disvm-editor --gc-worker --jit --stats native-wait.dis
./disvm-editor --gc-worker --gc-locked-poll --no-jit --stats native-wait.dis
```

For headless execution set SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy.
Native files/modules resolve beside the entry; compile/run it at package root.
The standalone console host accepts the same flags. --gc-inline is also checked.

The runtime captures scalar arguments and the complete result slot before any
wait. It saves the complete native call REG in the current Prog, publishes stack
roots and leaves native_world. The C callback then owns only native scalar locals
and performs OS sleep through the shared FlashVM thread port. It reenters the
world before writing the managed result cell. No managed memory is accessed by
this callback while it is inactive. Collectors can scan the saved stack and
process list while the native callback waits.

NATIVEGC reports native_releases==native_reentries==7 for a successful worker run:
four positive waits, two nonpositive calls and the final formatted print. The oversized call refuses before
release. native_gc_progress must be positive: retained cycle passes actually ran
inside released intervals. Queue owners/bytes must drain and both workers join.
The gate executes12 successes and2 managed-limit error retirements for each host,
covering inline/atomic/locked, interpreter/JIT and quanta1/2000. No timing threshold
is used as evidence for correctness or UI latency.

This blocks the sole Dis owner, while UI and GC OS workers remain independent.
It does not schedule other Limbo spawn processes on additional mutators. The
release API is for audited callbacks with fully published roots and no invisible
managed C-stack owners. Do not wrap arbitrary native read/write/extension bodies
with it: those need separate root/buffer/exception audits. Generic release/acquire,
concurrent marking and parallel shared-heap spawn remain separate work.
