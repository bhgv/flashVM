# Native cycle collector with real managed roots

[main.b](main.b) creates 12,000 unreachable self-cycles. Reference counting alone
cannot reclaim them. A separate live self-cycle and its payload must survive all
allocations and register/stack safepoints. The program raises on lost live data;
normal return allows the runtime to join the collector and release its native TLS.

From the matching installed native SDK, compile and execute:

```
./limbo-native -g -I include/limbo -o gc-worker.dis examples/gc-worker/main.b
./disvm-editor --gc-worker --jit --stats gc-worker.dis
./disvm-editor --gc-worker --no-jit --stats gc-worker.dis
```

For headless execution set `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy`. The console
runtime supports the same GC flags. This example creates no UI scene. Keep the
compiled entry beside the installed modules; imports resolve from its directory.
`--gc-inline` is the default comparison route. Do not use an old legacy-cell
compiler or mix the native-cell compiler/runtime ABI.

`PASS gc-worker` verifies live data. `NATIVEGC` is printed only after collector
join and reports actual retained `rungc` passes, active-register snapshots and
cycle-reclaimed objects. Counts depend on OS scheduling and request coalescing;
they are not fixed expected totals. Reclamation can lag creation because each
pass preserves the retained collector's adaptive work budget.

The test runs interpreter/JIT with quanta 31, 257 and 2000, and two instruction
limit failures. It requires positive real reclamation, snapshots and epochs,
matching pass accounting and collector join after managed error. The editor
variant additionally checks UI/Dis owner TLS retirement. Limbo processes remain
cooperative inside one Dis mutator; this example does not claim parallel spawn.
