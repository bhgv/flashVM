# Source → FIR → optimizer → SWO → linker → SWF

Run from the repository root. Termux: `pkg install git clang make cmake python`.
The generator is external bhgv/coco-r-C; its sources/runtime are not vendored.

```sh
export TMPDIR="${TMPDIR:-$PWD/build/tmp}"
mkdir -p "$TMPDIR"
git clone https://github.com/bhgv/coco-r-C.git "$TMPDIR/coco-r-C"
make -C "$TMPDIR/coco-r-C/sources" -f unix.mk
cmake -S fir/v1 -B build/fir1 \
  -DCOCOR_ROOT="$TMPDIR/coco-r-C" \
  -DCOCOR_EXECUTABLE="$TMPDIR/coco-r-C/sources/cocor"
cmake --build build/fir1
cmake -S linker -B build/linker
cmake --build build/linker
build/fir1/fir1_source as2 --input fir/v1/examples/as2/variables.as build/variables.fir --module variables
build/fir1/fir1 check build/variables.fir
build/fir1/fir1_opt build/variables.fir build/variables.opt.fir
build/fir1/fir1 object build/variables.opt.fir build/variables.swo
build/linker/swl archive build/library.swa build/variables.swo
build/linker/swl link build/variables.swf build/library.swa
FIR_SWL="$PWD/build/linker/swl" ctest --test-dir build/fir1 --output-on-failure
ctest --test-dir build/linker --output-on-failure
```

Expected trace: 36, 42. SWO1 is a target-profile AVM1 object, without a SWF
container. SWA1 includes whole objects. Multiple module initializers run in input
order; duplicate module names fail. Cross-module function calls/imports/exports
and their relocations are not implemented yet. FIR channels require explicit
`swl link ... --extensions ...` and installed host extensions.
[SVF/JPEG/WAV resources](../../../linker/ASSETS.md) are added by the linker,
never the compiler. Optimization is explicit; text format remains `FIR 1.0`.

[C IDE example](../../../linker/examples/ide/README.md) links all libraries into
one process. [API](../compiler/API.md). The verifier additionally checks indices,
table counts, arity and names of C-constructed IR. Source parsing still requires
external serialization; do not call the same parser concurrently without a
mutex. Complete languages, ABC, invoke/catch and block parameters remain future
work. Windows/physical Android were not tested in this series.
