# Staged native read/readn with real heap release

[main.b](main.b) reads a sliced array, fragmented stdin,100003 nonuniform binary
bytes, partial EOF and ordinary files. A second real Dis process stays blocked
on a channel with its own array and cyclic graph argument. Both processes check
their roots after I/O. The companion driver checks every echoed byte, including
NUL/high bytes and stage boundaries; untouched prefixes/tails must stay intact.
A write-only file tests actual port failure without modifying the managed buffer.

Compile/run from the matching installed native-cell SDK:

```
./limbo-native -g -I include/limbo -o native-read.dis examples/gc-native-read/main.b
python source/threading/test_native_read.py ./disvm-editor ./native-read.dis
```

The driver creates private regular files and a genuine delayed OS pipe. It feeds
all required bytes automatically and removes its own files after child/writer joins.
It selects dummy SDL/audio for headless execution. Running the module directly
without that fixture waits for stdin and requires its named input files. This is
an NC 64 managed example, separate from shared Dis 64 scalar-file admission.

Sys.read/readn capture the generated frame first. Both managed FD and array are
registered external roots (including a slice's backing graph). A native buffer
has at most 65536 bytes. The native file handle is captured while heap ownership
is active. The Dis owner then leaves native_world; OS reads write only the native
buffer. After reentry, descriptor/span are revalidated and only acknowledged bytes
are copied into the managed array. readn repeats bounded fragments; read returns
one fragment. EOF preserves the unfilled suffix. Root/pinning errors unwind with
native storage cleanup; port errors acknowledge a prior prefix or return-1.

NATIVEINPUT must show pins=0 and staging=0 after completion or managed-limit error.
Peak staging is 65536 in this fixture and normal committed bytes total 100032.
NATIVEGC requires matched native releases/reentries and actual cycle progress
inside released intervals. Each console/UI host runs 12 successes and 2 errors:
inline/atomic/locked, JIT/interpreter and quanta 31/2000. Delays are not a timing
oracle. Limbo spawn tasks remain cooperative on the sole Dis owner; OS I/O waits
release the heap for collectors and keep UI independent.
