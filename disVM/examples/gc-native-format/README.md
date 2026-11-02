# Formatted native output with live managed roots

[main.b](main.b) fills an actual stdout pipe using both `Sys.print` and
`Sys.fprint`. Each message contains a managed UTF-8 string and an integer
variadic argument. The driver independently constructs every expected output
byte and all file rows. It also checks read-only/standard-input failures,
unchanged source arrays, a live cyclic graph and a channel-blocked process's
own locals. All generated files and pipes belong to a private temporary tree.

Compile and test from the matching installed native-cell SDK root:

```sh
./limbo-native -g -I include/limbo -o native-format.dis examples/gc-native-format/main.b
python3 source/threading/test_native_format.py ./disvm-editor ./native-format.dis
```

The Linux driver restricts only its own pipe to less than the payload size,
waits for a stderr readiness handshake, briefly leaves stdout undrained and then
drains it. No global kernel setting changes. The headless UI host uses SDL dummy
ports; the same test runs against the standalone console executable.

Formatting uses the existing 4096-byte message contract on the exclusive Dis
heap owner. The complete resulting C stack buffer is immutable during native
I/O. The FD is externally rooted, its native handle is captured before leaving,
and the native call REG is published to the saved managed stack. Only native
bytes/handles reach the port while outside the heap. Reentry precedes root
retirement, managed result publication and error reporting. No managed format
string or variadic owner is read during the released interval. Ordinary short
write and zero-byte access semantics remain in force; this adds no file atomicity
or durability guarantee. Oversize formatting retains its existing managed error.

`NATIVEFORMATTED` must report zero remaining FD pins, a completed attempt count
and one release for each attempt in worker mode (zero in the inline reference).
GC counters require matched reentries, real cycle progress and joined collectors;
UI mode also checks native context retirement. Per host, 12 successful runs and
2 post-output instruction-limit retirements cover interpreter/JIT, three GC
routes and two scheduling quanta. Timing is not a correctness/speedup threshold.

This does not make Limbo `spawn` parallel. There is still one managed Dis mutator;
UI and collectors are actual separate native owners. Generic native modules,
loader callbacks, cancellation, scheduler concurrency and graph write barriers
remain separate audits.
