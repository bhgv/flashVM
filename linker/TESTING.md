# SWL verification

```sh
cmake -S linker -B build/linker
cmake --build build/linker
ctest --test-dir build/linker --output-on-failure
```

96 subprocess checks need no compiler, Coco/R or VM. Python stdlib independently
validates object headers/checksums, SWF tags, RECT/shape edges, bitmap matrix/JPEG
bytes, all 16 PCM profiles, exports, placements and sound events. Invalid inputs
preserve existing output. [C embedding test](tests/embed.c) exercises rollback
after partial archive failure and re-emission after writer failure. Optionally
`FIR_FLASHVM_RUNNER` supplies a host to load SWF and execute two DoAction inputs
in order. [Full C example](examples/ide/README.md). Ordinary and ASan/UBSan tests
passed; sanitizer restrictions and full evidence are in
[FIR TESTING](../fir/v1/doc/TESTING.md). Loading is not a physical display/audio
device test.
