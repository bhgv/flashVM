# Direct Dis backend, FIR implementation 0.17.0

[Typed table Dis extension / Таблицы и нативные вызовы](TABLES_DIS.md) · [Русский](TABLES_DIS.ru.md). The scalar CFG contract below remains separate from the single-block hosted table path.


[Русский](DIS_BACKEND.ru.md) · [Compiler implementation](../compiler/dis.c) ·
[C99 API](../compiler/API.md) · [Commented tutorial](../examples/dis/README.md) ·
[DisVM runner](../../../disVM/runtime/README.md).

Our compiler now writes an executable NC64 `.dis` directly from verified FIR.
It does not invoke `limbo-native`, the Inferno assembler, Python, LLVM or an
external VM to generate code. Both source and FIR parsers remain Coco/R-generated
C99. The existing SWO/SWL/SWF8 path remains available as a separate target.

The scalar capture target now admits verified multi-block CFG: i32/bool/f64/i64 constants,
scalar copy, i32 wrapping and binary64 +/−/×/÷ arithmetic, signed less-than/equality, bool branches,
unconditional jumps, captured scalar trace and return. Forward and backward
edges are relocated from FIR block identities to final **Dis instruction PCs**,
including after optimization and across variable-width file operands.

The Coco/R numeric Limbo adapter additionally accepts terminal decision trees:
after its ordinary prelude, either `return expression;` or a braced
`if (expression relation expression) { result } else { result }`, recursively.
Relations are `<`, `==`, `>`, `!=`, `<=`, `>=`. Every arm returns. Compiler 0.22 admits lexical
declarations/assignments inside arms; continuation after a decision remains unsupported.
See [terminal-arm semantics](TERMINAL_ARMS.md).
Consequently no phi/join environment is silently invented. Source limits remain
64 KiB, 1000 values, shared nesting depth 64; at most 127 decisions introduce
255 blocks. `-g DEBUG.json` records comparison-operator and `if` keyword spans.

The existing numeric-only AS1/2/3 adapters now also reach this Dis backend through
explicit f64 operations; their source profile metadata remains intact. Dynamic
AS coercion (`as.add`, `as.to_number`), strings/channels, imports and function
exports remain unsupported and fail preflight. General Limbo functions, mutable
source loops, SSA phi and Dis SBL remain separate work.
Manual FIR cycles can execute but need an explicit runtime instruction limit;
this does not claim source while/for lowering or loop-carried SSA support.

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

In this harness, each **live static trace instruction** has a capture slot in
physical FIR order. For N traces, the first N eight-byte cells hold values; the
next N cells hold execution flags, initially zero. A visited trace writes its
value and flag one. An unvisited slot stays zero with flag zero, so a taken
`trace 0` is distinguishable. Revisited slots retain the latest value; this is
not an ordered event log. Raw and optimized artifacts can have different N
because unreachable traces disappear. Count live O_TRACE instructions in the
post-optimization module, available to the existing `after` snapshot callback;
publish that metadata only if compilation succeeds.

Linear clients reading the first N values keep their offsets. Clients consuming
CFG results must also read flags. The private constant pool follows the capture
area; it is not trace output. No new Dis tag/opcode/ABI is introduced. Ordinary
execution without inspection does not print captured values or call Sys print.

`disvm --inspect-words OFFSET COUNT` explicitly requests a post-execution scalar
snapshot. OFFSET is an aligned decimal byte offset; COUNT is 1..4096 native
words. Before execution the owner checks the actual entry-instance data range
and rejects pointer-bearing descriptors. It retains that instance's scalar data
until report/error cleanup, so it reads executed writes rather than pristine
module constants or a dead process frame. Output is `DATA byte_offset signed_word`.

## Bounded execution, component 0.9.0

`disvm --max-instructions COUNT` accepts one decimal value in 1..2147483647.
At the full instruction boundary, after the GC poll and before C/native execution,
the owner refuses the next dispatch once COUNT instructions have been admitted.
Fallback is counted once; successful native leaves alone increment native stats.
This is a total across processes/scheduler slices, not reset by stdin/UI progress;
it is not a wall-clock timeout. The existing busy-work guard still applies.

At the exact budget, a completed program succeeds. A cycle exceeding the budget
fails with `instruction limit exceeded`, reports its current unexecuted PC and,
with `--stats`, the exact admitted count. No incomplete DATA snapshot is printed;
the extra inspection retain is retired through the error path. Without the option,
the prior runtime policy remains active. No remote debugger protocol or breakpoint
is implied by this local CLI execution limit.

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
SSA joins/source loops, f64/strings, source channels, broader Limbo/AS lowering, Dis
linking and SBL/source-PC mapping. Existing source `-g DEBUG.json` origins remain
available but are not automatically a Dis `.sbl`. The complete graphical IDE
still uses temporary source-built bootstrap modules until its language subset
can be lowered by our compiler.

[Terminal decision tutorial](../examples/dis/README.md#terminal-decisions-and-capture-flags)
contains a three-slot path example. Tests additionally cover six signed source
relations on both outcomes, physical backward edges, PCs above 8191, 255 blocks,
quota boundaries/cycles, and caller-constructed invalid IR. Live blocks must be
contiguous and begin with the declared entry, including after removed prefixes;
this invariant protects both SWO and Dis emitters. The structural Coco/R frontend
also distinguishes block declarations after a return from actual label operands.

## Binary64 and typed observations (compiler 0.9.0, runtime 0.10.0)

`const.f64`, f64 `copy` and `f64.add/sub/mul/div` use existing MOVF and real
arithmetic instructions. The constants' IEEE bits are copied through uint64_t
and emitted big-endian with DEFL, preserving negative zero. Finite source
constants are still required; operations can produce infinity or NaN. Division
is real division, including signed-zero/invalid IEEE results, not integer division.
No i32 normalization follows a real operation. NaN payload identity is unspecified.

Each real trace occupies the same eight-byte capture cell as an integer trace.
The format has no automatic type tags: callers know the trace types from FIR.
Use `--inspect-reals OFFSET COUNT` for a homogeneous binary64 range. Output is
`REAL byte_offset value`, with 17 significant decimal digits, signed zero and
Inf/NaN class retained. This option shares range/descriptor/retention/error checks
with `--inspect-words`; the two views cannot be requested together. Executed
flags remain native integer 0/1 and must be read as words. Mixed traces require
separate range observations; interpreting an integer cell as a real performs a
bit reinterpretation, never a numeric cast. No partial REAL report is printed
on a VM error or exhausted instruction quota.

See [the numeric AS3 example](../examples/dis/README.md#binary64-as-source).

Our Coco/R Limbo adapter now additionally admits a homogeneous `value(): real`
profile: decimal real constants, typed/inferred real locals, lexical scopes,
assignments, + - * / and return. No mixed types/casts or real comparisons/CFG
are implied. [Exact grammar coverage](SOURCE_SYNTAX.md#homogeneous-limbo-real-090).

Наш Limbo real — ограниченный однородный профиль с явными real-литералами;
обычные int-функции и их терминальные ветвления сохраняют прежнюю семантику.
[Комментированный пример](../examples/dis/README.ru.md#вещественный-limbo).

[Big CFG: contract and example / Контракт и пример](BIG_CFG.md) · [Русский](BIG_CFG.ru.md).

[FIR 0.18: big comparisons and terminal source if/else / Сравнения и решения исходника](BIG_DECISIONS.md) · [Русский](BIG_DECISIONS.ru.md).
