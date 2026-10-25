# Direct Dis backend, FIR implementation 0.6.0

[Русский](DIS_BACKEND.ru.md) · [Compiler implementation](../compiler/dis.c) ·
[C99 API](../compiler/API.md) · [Commented tutorial](../examples/dis/README.md) ·
[DisVM runner](../../../disVM/runtime/README.md).

Our compiler now writes an executable NC64 `.dis` directly from verified FIR.
It does not invoke `limbo-native`, the Inferno assembler, Python, LLVM or an
external VM to generate code. Both source and FIR parsers remain Coco/R-generated
C99. The existing SWO/SWL/SWF8 path remains available as a separate target.

This first target is an **integer capture harness**, not complete Limbo module
compilation. It runs one FIR `@main() -> void` with one block. It admits i32
constants, i32 copy, wrapping addition/subtraction/multiplication, i32 trace and
return. It rejects AVM1 numeric profiles, other types, branch/loop CFGs, channels,
imports, function exports and other operations before writing output. AS1/2/3
source lowering still uses binary64 and therefore does not target this backend.
Broad Limbo syntax recognition in `compiler/limbo` does not imply lowering it.

## Console pipeline

From the repository root, build FIR with the selected owned Coco/R C generator:

```sh
cmake -S fir/v1 -B build/fir \
  -DCOCOR_ROOT="$PWD/tools/coco-r-C/upstream" \
  -DCOCOR_EXECUTABLE="$PWD/build/cocor/cocor"
cmake --build build/fir
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/dis/main.b build/answer.fir
build/fir/fir1 check build/answer.fir
build/fir/fir1 dis build/answer.fir build/answer.dis
build/editor/disvm --stats --jit --inspect-words 0 1 build/answer.dis
build/editor/disvm --stats --no-jit --inspect-words 0 1 build/answer.dis
```

Build `disvm-editor` or `disvm-runtime` separately through the workspace scripts;
`build/editor` is just the example runtime build directory. Expected observation:
`DATA 0 2147483647`. Actual instruction/native counters appear on stderr; native
is zero in interpreter mode. A JIT fallback is not counted as a native leaf.

Optimization is explicit: run `fir1 opt INPUT.fir OPT.fir`, then `fir1 dis OPT.fir
OUTPUT.dis`. The API can also request optimization through FirRunOptions.
All input/backend checks finish in temporary storage before the CLI opens an
existing output path. Unsupported input leaves its old bytes intact. Final
filesystem write failures can still leave a prefix; there is no rollback claim.

## Exact target and observations

The target uses existing Dis instructions, data records and descriptors with
experimental **NC64 magic 819264**. This matches our native Linux AMD64 runner;
it is not classic four-byte Dis or the proposed standard ABI64. No new Dis tag,
opcode, instruction encoding or SWF container is introduced.

NC64 WORD is eight bytes, while FIR i32 is signed 32-bit. Every arithmetic
instruction is followed by normalization:

```text
((native_result & 0xffffffff) ^ 0x80000000) - 0x80000000
```

This normalizes intermediate results, not only the final observed value.
The product of two admitted signed i32 values fits the native 64-bit range.
Constants use existing big-endian DEFL records; code operands use existing
signed-30-bit Dis encoding. Frame size, offsets, instruction counts and data
extent are bounded by the verified FIR limits. No pointers occur in module data
or local-value maps; Dis owns the ordinary entry-frame header and lifetime.

In this explicit harness, `trace` **captures**, rather than streams, its i32
value into module data: trace zero at byte 0, trace one at byte 8, and so on.
Running without `--inspect-words` executes the program but does not print these
values. There is no Sys import or print service hidden in generated code.
Values are read only after all Dis processes finish. This observation contract
is deliberately narrower than live SWF trace; it does not add a debugger
breakpoint, source stepping or a new module ABI.

`disvm --inspect-words OFFSET COUNT` explicitly requests a post-execution scalar
snapshot. OFFSET is an aligned decimal byte offset; COUNT is 1..4096 native
words. Before execution the owner checks the actual entry-instance data range
and rejects pointer-bearing descriptors. It retains that instance's scalar data
until report/error cleanup, so it reads executed writes rather than pristine
module constants or a dead process frame. Output is `DATA byte_offset signed_word`.

## Library and IDE build service

Link `fir1_source_api` and `fir1_ir`; include `fir.h`. Use `fir_source` to stage
Limbo numeric source as FIR, rewind, then call `fir_run(FIR_DIS, ...)`. An existing
verified module can be passed directly to `fir_dis`. Streams and diagnostics
remain caller-owned; functions return 1/0 and never close the streams or exit.
NULL diagnostics are admitted. Backend coverage errors write nothing; I/O errors
can leave a prefix in a directly supplied output stream.

The [C99 client](../examples/dis/main.c) demonstrates private staging and cleanup
without child processes. IDE compilation belongs on its native build worker;
return copied outcomes through the service channel. This does not move native
compiler implementation or native pointers into the deployed Dis UI process.
Serialize calls to each generated parser. Separate caller-owned FIR modules can
be verified/emitted without parser globals or shared backend state.

## Verification and remaining work

Compiler-only CTest covers deterministic NC64 emission, API ownership/NULL/error
recovery and coverage rejection without destination overwrite. Optional runtime
tests are enabled with DVM_FIR_COMPILER and DVM_FIR_SOURCE_COMPILER paths. They
run 90 dependent arithmetic operations against a separate signed-i32 oracle,
raw and optimized, in interpreter/JIT and UBSan configurations; they also execute
our actual Limbo source/scopes pipeline and reject malformed inspection ranges
and pointer-bearing module data. No temporary Limbo compiler emits these programs.

Next required stages include typed functions/module signatures and imports,
branches/loops, f64/strings, source channels, broader Limbo/AS lowering, Dis
linking and SBL/source-PC mapping. Existing source `-g DEBUG.json` origins remain
available but are not automatically a Dis `.sbl`. The complete graphical IDE
still uses temporary source-built bootstrap modules until its language subset
can be lowered by our compiler.
