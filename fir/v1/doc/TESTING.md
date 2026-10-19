# Testing

[Русский](TESTING.ru.md). [check.py](../tests/check.py) is the compiler acceptance
suite. CTest runs syntax/semantic negatives, all examples before/after peephole,
round trips and an independent SWF/action-framing reader. It checks duplicate
SSA, dominance, undefined labels, invalid types/version/UTF-8/comments, bounds,
unsupported source constructs, branch range and output preservation.

Actual runtime acceptance is optional and explicitly reported separately:

```sh
cc -std=c99 -IflashVM/include -c fir/v1/tests/run_flashvm.c -o /tmp/fir-run.o
c++ /tmp/fir-run.o VM_BUILD/libflashvm.a VM_BUILD/libfvm_agg.a \
  VM_BUILD/libfvm_zlib.a -lpthread -ldl -lm -o /tmp/fir-run
FIR_FLASHVM_RUNNER=/tmp/fir-run ctest --test-dir build/fir1 --output-on-failure
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
