# Source → FIR → SWF

Run from the repository root. Termux needs `git clang make cmake python`.
The external generator's sources/runtime are not vendored into flashVM.

```sh
export TMPDIR="${TMPDIR:-$PWD/build/tmp}"
mkdir -p "$TMPDIR"
git clone https://github.com/bhgv/coco-r-C.git "$TMPDIR/coco-r-C"
make -C "$TMPDIR/coco-r-C/sources" -f unix.mk
cmake -S fir/v1 -B build/fir1 \
  -DCOCOR_ROOT="$TMPDIR/coco-r-C" \
  -DCOCOR_EXECUTABLE="$TMPDIR/coco-r-C/sources/cocor"
cmake --build build/fir1
ctest --test-dir build/fir1 --output-on-failure
build/fir1/fir1_source as2 --input fir/v1/examples/as2/variables.as build/variables.fir
build/fir1/fir1 check build/variables.fir
build/fir1/fir1 opt build/variables.fir build/variables.opt.fir
build/fir1/fir1 swf build/variables.opt.fir build/variables.swf
build/fir1/fir1 actions build/variables.opt.fir build/variables.actions
build/fir1/fir1 --help
build/fir1/fir1_source --help
```

Expected trace: 36 then 42. This is one FWS8 frame with AVM1 and no graphics.
For AS1/AS3 change the profile and example directory. For Limbo use `limbo`
and `fir/v1/examples/limbo/variables.b` (output 36). AS3 is a numeric AVM1
compatibility profile, without ABC/AVM2. `actions` writes raw AVM1 records,
not a linker object. Optimization is explicit; `swf` does not run it.

Syntax frontends `compiler/{as1,as2,as3,limbo}` accept broader syntax and build
separately. Their `--help` does not promise FIR/SWF output. The FIR parser is
`compiler/fir1.atg`, tutorial source parser `compiler/src1.atg`; semantics are
C99 `compiler/lower.c`. Extend grammar, semantics, negative tests and docs together.

CTest without `FIR_FLASHVM_RUNNER` does not execute SWF; see
[TESTING.md](TESTING.md) for VM integration. The format remains `FIR 1.0`.
Block parameters, function arguments, invoke/catch, complete AS/Limbo and ABC
remain unimplemented; this does not complete the entire compiler roadmap.
