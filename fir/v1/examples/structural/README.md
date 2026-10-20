# FIR function and block parameters

[main.fir](main.fir) is a readable structural sum-loop example using block
parameters instead of phi. It demonstrates signatures, explicit edge arguments,
file/loc and export. Parsing does not compute a sum, build an SSA graph or emit
SWO. The current executable backend still rejects this profile.

```sh
cmake -S . -B build -DCOCOR_ROOT=/host/coco-r-C \
  -DCOCOR_EXECUTABLE=/host/coco-r-C/sources/cocor
cmake --build build
build/fir1 syntax examples/structural/main.fir
build/frontend/firsyn_parse examples/structural/main.fir
ctest --test-dir build --output-on-failure
```

Run these commands from fir/v1. Expected result: status 0, no computed output or
file. Requirements: C99, CMake 3.16+, external HOST Coco/R with matching frames;
Python 3 for tests. A C host uses firsyn_check on its borrowed byte buffer or
firsyn_read on FILE. Both are synchronous, return boolean status and never close
input. The host owns cleanup/diagnostics; serialize access to each parser.
Variable ordering, edge type agreement, dominance and opcode effects need a
separate verifier. Never pass syntax acceptance to an optimizer as a verified
FirModule. Object generation requires executable-profile verification; a negative
test checks that unsupported structured input preserves an existing output file.

[API/grammar contract](../../frontend/README.md) ·
[Catalog](../README.md) · [Build](../../../../BUILDING.md).
