# SWL verification

```sh
cmake -S linker -B build/linker
cmake --build build/linker
ctest --test-dir build/linker --output-on-failure
```

The format subprocess checks need no compiler, Coco/R or VM. Python stdlib independently
validates object headers/checksums, SWF tags, RECT/shape edges, bitmap matrix/JPEG
bytes, all 16 PCM profiles, exports, placements and sound events. Invalid inputs
preserve existing output. [C embedding test](tests/embed.c) exercises rollback
after partial archive failure and re-emission after writer failure. Optionally
`FIR_FLASHVM_RUNNER` supplies a host to load SWF and execute two DoAction inputs
in order. [Full C example](examples/ide/README.md). Earlier ordinary and ASan/UBSan results,
sanitizer restrictions and historical evidence are in
[FIR TESTING](../fir/v1/doc/TESTING.md). Loading is not a physical display/audio
device test.

## SWL 0.3.10 inspection and stage controls

Three native CTest entries pass: formats/CLI, existing embedding, and new
inspection embedding. The Python CLI suite covers every truncated prefix of a
library, late corruption and no partial metadata, exact stage RECT/rate values
at minimum/maximum bounds, invalid stage values and resource operands named
`--extensions`. The C inspection suite separately checks cancellation and
callback suppression. The native tutorial also builds/runs after copying both
SDK and client into renamed directories containing spaces.

The CLI and inspection tutorial cross-compile/link for Linux ARM64, Linux
RISC-V64 and Windows x64 using the explicit SDK recipe and existing host wrapper
adaptation documented in [toolchain validation](../toolchains/VALIDATION.md).
No foreign executables were run. These checks do not close roadmap stage 9.
No fresh sanitizer or full VM suite is claimed for this series.
