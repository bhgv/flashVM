# Immutable native output snapshots under real backpressure

[main.b](main.b) writes a 100003-byte slice to a file and stdout. Its payload has
NUL/high-byte and stage-edge markers. Prefix/tail sentinels, a cyclic graph and
another channel-blocked process's array must remain valid. The driver checks all
bytes in both outputs and verifies that a read-only file stays unchanged after
an attempted write. Ordinary zero-write descriptor/access checks are preserved.

From the matching installed native-cell SDK:

```
./limbo-native -g -I include/limbo -o native-write.dis examples/gc-native-write/main.b
python source/threading/test_native_write.py ./disvm-editor ./native-write.dis
```

The Linux driver creates a private output directory. It sizes only its own stdout
pipe, checks that capacity is smaller than the payload, waits for a stderr-ready
handshake and briefly leaves stdout undrained. This causes genuine backpressure
while GC can scan saved roots. It then drains stdout and verifies every byte and
worker retirement. Kernel-global settings and user files are not changed.

Positive Sys.write requests validate/pin the original FD and source array, copy
at most 65536 bytes into a native buffer, then leave the heap. The port sees that
immutable native snapshot. Reentry precedes managed result publication and root
retirement. Source memory is never passed to OS I/O during a released interval.
Sys.write returns an acknowledged fragment; callers must loop on short writes,
as the example and IDE file-service library do. No file atomicity/durability or
rollback guarantee is added. Zero-byte file operations retain legacy active-owner
access checks. Formatted print/fprint and generic C modules need separate audits.

NATIVEOUTPUT requires pins=0, staging=0, peak 65536 and 200006 acknowledged bytes for
normal fixture runs. NATIVEGC must report matched release/reentry and real cycle
progress. Each console/UI host executes 12 successes and 2 managed-error joins,
covering inline/atomic/locked, interpreter/JIT and quanta 31/2000. No speedup/timing
threshold decides correctness. Parallel shared-heap mutators remain closed.
