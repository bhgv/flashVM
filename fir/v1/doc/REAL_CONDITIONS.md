# Ordered real conditions — FIR 0.54 / source CLI 0.21

[Русский](REAL_CONDITIONS.ru.md) · [Line-commented Limbo tutorial](../examples/dis/real-conditions.b) · [Line-commented unordered FIR](../examples/dis/real-unordered.fir) · [Coverage](COVERAGE.md) · [Typed int/bool predicates](TYPED_PREDICATES.md)

The homogeneous Limbo `value(): real` profile now supports terminal `if (...) { ... } else { ... }` decisions, including bounded nested decisions and arm-local aliases/bindings. Every arm still returns. Arithmetic operands remain homogeneous binary64: there is no implicit int conversion, numeric truthiness, first-class real comparison local, loop/join extension or general module-call ABI in this profile. The authoritative Coco/R source grammar invokes the same C99 semantic actions for CLI and library users.

| Source relation | FIR predicate |
| --- | --- |
| a == b | f64.eq a, b |
| a != b | invert the f64.eq branch edges |
| a < b | f64.lt a, b |
| a > b | f64.lt b, a |
| a <= b | f64.le a, b |
| a >= b | f64.le b, a |

Both operands are evaluated once, left before right; swapping only affects their already computed SSA identities. `f64.eq`, `f64.lt` and `f64.le` have append-only IDs and signature f64/f64 → bool. They are pure, total predicates with normalized 0/1 results. Every ordered predicate is false if either operand is NaN. Equality accepts +0/-0 and equal infinities; inequality is true for NaN. In particular, <= is not implemented as inverted >. NaN payloads and floating status flags are not exposed by this bounded source profile.

FIR literals remain finite. Nonzero representable subnormals, including the smallest binary64 value, are accepted even when the host strtod reports ERANGE. Values rounding entirely to zero and overflow are rejected with staged output retained. The same policy is used by source adapters and the FIR reader; AS numeric adapters benefit from the literal correction without gaining Limbo condition syntax. NaN/infinity can still arise during execution, for example 0.0/0.0 or overflow.

## Backend and optimization contracts

The verifier checks exact operand/result types and arity before emission, and rejects forged nonfinite literal payloads. Constant comparisons fold using binary64 relations. Identical pure predicates may be reused within a block, and unused ones may be removed while retaining checked producer effects. The optimizer does not rewrite arbitrary `x == x` to true for real: x may be NaN. Existing strict arithmetic keeps its order.

Scalar and hosted NC64 Dis emitters use existing BEQF/BLTF/BLEF and materialize normalized bools. Raw hosted FIR also admits binary64 constants, arithmetic and copies; those frame/data cells are plain scalars, never GC roots or retaining pointer copies. The C99 reference runner supports these values independently. Table-backed raw FIR still has its existing trace-type boundary: capture a bool predicate there, rather than requesting a new hosted real trace ABI. No Dis instruction, native module ABI, GC geometry or shared-heap pointer is added.

SWO emission preserves IEEE predicates despite the current loaded AVM1 player's NaN-pair equality policy. Less2 leaves unordered as undefined. The compiler normalizes `<` with two Not actions; `<=` compares the reverse Less2 result with false, so undefined remains rejected. Equality combines both ordered directions with Boolean And. Operands are loaded from their already computed SSA slots; expressions are not reevaluated. Existing dynamic AS equality behavior is unchanged. SWL 0.3.14 validates the standard And action (0x10); SWO1/SWF8 formats remain unchanged.

## Run the examples

From the repository root with matching tools:

```sh
export TMPDIR="$PWD/build/tmp"
mkdir -p "$TMPDIR"
build/fir/fir1_source limbo --input fir/v1/examples/dis/real-conditions.b build/real-condition.fir -g build/real-condition.json
build/fir/fir1 opt build/real-condition.fir build/real-condition.opt.fir
build/fir/fir1 dis build/real-condition.fir build/real-condition.dis
build/runtime/disvm --jit --quantum 1 --stats --inspect-words 0 4 build/real-condition.dis
build/runtime/disvm --no-jit --quantum 1 --stats --inspect-words 0 4 build/real-condition.dis
build/runtime/disvm --jit --inspect-reals 0 2 build/real-condition.dis
build/fir/fir1 object build/real-condition.fir build/real-condition.swo
build/swf-linker/swl link build/real-condition.swf build/real-condition.swo
```

The unoptimized Limbo example has two capture values followed by two validity flags. Its first real is exactly 42.5, the inactive second capture is zero, and flags are 1/0. `--inspect-words` exposes raw binary64 bits; `--inspect-reals` interprets those bits as numbers in a separate invocation. Repeat emission/execution for optimized FIR, counting remaining trace sites rather than assuming the same slot count. The live result remains 42.5.

For `real-unordered.fir`, run the same FIR/Dis/object/linker commands directly, skipping source compilation. Its three bool captures are 0/0/0 followed by flags 1/1/1; use `--inspect-words 0 6`. A trace-capable SWF host must report three false values (text spelling depends on the host). Opening a movie alone does not establish execution. Every nonblank source line in both examples is commented. The executable reader and syntax frontend both admit a separated // comment after FIR 1.0. Numeric SWF trace is display text and can round a binary64 value; exact bits are checked through Dis inspection, while SWF predicate results are checked exactly.

Installed SDK acceptance relinks a fresh C99 client, compares source-library and CLI Dis bytes, and executes raw/optimized real decisions in interpreter and native modes with exact bit/validity/counter checks. The source API remains serialized and blocking; an IDE should invoke it on its build worker and return copied artifacts through its service channel. Tests cover unordered/scalar/hosted paths, 384 finite source relations, nonfinite execution, subnormals in all AS adapters, lazy terminal arms, aliases, staged rejection and exact UTF-8 relation spans. Native counters establish host dispatch; hardware ARM64 and full compiler/IDE acceptance remain separate.

[Lazy numeric conditions / Ленивые числовые условия — FIR 0.55](NUMERIC_LOGIC.md) · [Русский](NUMERIC_LOGIC.ru.md).
