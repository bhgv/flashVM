# Sys.millisec in the native Dis runtime

[Русский](MILLISEC.ru.md)

`sys->millisec()` now reads a real monotonic host clock in both the interpreter
and JIT, including the console `disvm` executable without SDL or Flash. Its epoch
is arbitrary: use differences for durations, never as calendar time. Resolution
and suspend behavior depend on the OS; the counter need not change each call.
The result uses the current runtime's WORD width and wraps as that bit pattern.
Do not assume an infinite nonwrapping signed value or a particular boot epoch.

The bridge copies/validates the generated Sys caller frame using the existing
`dvm_sys_capture` helper before sampling. OS failure raises a normal VM error
without publishing a bogus result. It allocates no heap memory, invokes no user
callbacks and performs no scheduler wait. Calls do not depend on SWF frames.

Implementation is split into `runtime/clock_sys.c` and the independent C99 native
port `host/clock_port.c`, declared by `include/disvm/clock_port.h`. Linux/Android
use CLOCK_MONOTONIC, macOS uses mach_absolute_time, Windows uses GetTickCount64.
There is no wall-clock/CPU-time fallback. The generic port returns an unsigned
64-bit sample, is safe for independent native callers and preserves its output
on refusal. Native Linux x64 is executed here; other port bodies are not claimed
as physical platform acceptance.

The installed runtime/editor package includes a self-contained probe:

```sh
./disvm --jit examples/millisec/main.dis
./disvm --no-jit examples/millisec/main.dis
```

It makes 2001 actual Sys calls and checks nonnegative elapsed differences in that bounded
interval, including ordinary signed counter wrap. The native port test checks 10000 ordered samples, null refusal and
advancement across a real 20ms host wait. This is not a precision/latency benchmark.
The `.b` example and source reference files are included in the SDK. This adds no
new Sys module signature, native file-call admission or Dis64/SWF format ID.
