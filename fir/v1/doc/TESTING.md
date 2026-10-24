# Testing

[Русский](TESTING.ru.md). [check.py](../tests/check.py) is the compiler acceptance
suite. CTest runs syntax/semantic negatives, all examples before/after peephole,
round trips and an independent SWF/action-framing reader. It checks duplicate
SSA, dominance, undefined labels, invalid types/version/UTF-8/comments, bounds,
unsupported source constructs, branch range and output preservation.

Actual runtime acceptance is optional and explicitly reported separately:

```sh
cc -std=c99 -IflashVM/include -c fir/v1/tests/run_flashvm.c -o /tmp/fir-run.o
c++ /tmp/fir-run.o VM_BUILD/libflashvm.a \
  VM_BUILD/libfvm_zlib.a -lpthread -ldl -lm -o /tmp/fir-run
FIR_SWL="$PWD/build/linker/swl" FIR_FLASHVM_RUNNER=/tmp/fir-run ctest --test-dir build/fir1 --output-on-failure
```

`VM_BUILD` is an existing compatible Linux static FlashVM build directory;
platform/library options may require its corresponding extra dependencies.
The host installs extensions and an ActionTrace callback, loads the SWF and
executes DoAction through the public C API. Expected traces are asserted both
before and after optimisation, including 16 full-width signed multiplication
pairs, dynamic concatenation, double values, both branch arms and channel transfer.
The host is not linked into the compiler and is not required for detached builds.
Without FIR_FLASHVM_RUNNER, no runtime execution claim is made by CTest.

For sanitizers configure a separate build with compiler/linker flags
`-fsanitize=address,undefined -fno-omit-frame-pointer` and run the same suite.
Termux/Windows examples are instructions, not fresh physical-device evidence.

## v0.611 evidence

Linux C99 build: 335 subprocess checks with the real FlashVM host passed,
both unoptimised and optimised movies. ASan/UBSan: the same 335 checks passed
with `ASAN_OPTIONS=detect_leaks=0`; LeakSanitizer is unavailable under this
execution environment's process tracing. This is not a leak-check claim.
Renamed detached build (path containing spaces) and compiler-local build passed.
Root plus six local viewer launchers served their home, Russian format guide,
channel tutorial and grammar over loopback HTTP; all FIR Markdown links resolved.
External Coco/R revision used: `b91b65233c264d92c63a5c8cd855e7fc784bca53`.

## Compiler series 0.2.2

Linux, GCC 13.3, C99: 472 subprocess checks with the real FlashVM host passed,
before and after optimization. The same 472 passed with ASan/UBSan for FIR and
the source adapter. This environment disables ASan stack instrumentation and
LSan; heap/global ASan and UBSan were exercised, not leak or stack checking.
All four syntax-frontend CTest suites passed. Detached renamed AS1, AS2, AS3,
Limbo and FIR copies built and tested in paths containing spaces. All 512 local
Markdown links resolved. Coco/R revision:
`b91b65233c264d92c63a5c8cd855e7fc784bca53`. No physical Android test was run.
Without /tmp, set TMPDIR to an existing writable directory; temporary files use
mkstemp and are immediately unlinked, leaving only their open streams.

## SWO toolchain 0.3.2 / SWL 0.3.1

Current compiler-only CTest validates SWO1. Set `FIR_SWL` to the absolute `swl`
path for SWF assembly; also set `FIR_FLASHVM_RUNNER` for execution. A runner
without a linker is invalid. Verification/optimization precede object generation;
only SWL creates the SWF container.

Linux GCC 13.3 C99: all 15 CTest tests passed (FIR 3, frontends 9, linker 2,
IDE 1), in ordinary and ASan/UBSan builds. FIR: 558 subprocess checks with the
real FlashVM runner before/after optimization. SWL: 96 object/library/resource
and rejection checks, including all 16 PCM rate/channel/sample-width profiles.
C embedding tests cover repeated calls after errors, linking every parser,
snapshots, invalid C-constructed IR, archive rollback and re-emission after
callback failure. The IDE SWF traces 42 and 36 through FlashVM. Detached renamed
AS1/AS2/AS3/Limbo/FIR/SWL copies built and tested without hidden sibling source
dependencies. The sanitizer host disables LSan and ASan stack instrumentation;
heap/global ASan and UBSan were exercised, not leaks/stack. The resource SWF
loaded in the VM; physical rendering/audio-device playback was not tested.
Coco/R revision: `b91b65233c264d92c63a5c8cd855e7fc784bca53`.

[0.5.3 lexical-source and IDE continuation acceptance](../../../ide/dis/SERIES_0_3_1.md).
