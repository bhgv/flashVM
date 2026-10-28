# Typed predicates — FIR 0.53 / source CLI 0.20

[Русский](TYPED_PREDICATES.ru.md) · [Line-commented Limbo example](../examples/dis/typed-predicates.b) · [Line-commented FIR example](../examples/dis/booleans.fir) · [Comparison syntax](COMPARISON_VALUES.md) · [Short circuit](SHORT_CIRCUIT.md) · [Coverage](COVERAGE.md)

The Limbo big/table source profile keeps inferred bool values distinct from int and big. All six same-type int relations now use i32.eq or i32.lt.signed directly, swapping the already evaluated operands for > or <= and applying bool.not for inverse relations. No int operand widens to big for comparison. Evaluation remains left to right, once per operand. Big and string comparisons keep their existing full-width/content semantics; an inverse result now uses bool.not.

Two append-only FIR operations carry strict signatures:

| Operation | Inputs | Result | Effects |
| --- | --- | --- | --- |
| bool.not | bool | bool | none |
| bool.eq | bool, bool | bool | none |

Both inputs and results are normalized 0/1. These operations do not accept int, big, real, string or any by implicit conversion. Bool ordering and implicit truthiness remain errors. There is no new source bool declaration keyword: comparisons and existing owner operations infer the type. `big flag` remains an explicit value conversion. Source parsers continue to be generated from the authoritative Coco/R grammar; no handwritten parser is added.

The verifier checks arity, dominance, exact operand/result types and normalized constants before emission. Unknown future opcodes retain conservative effects. The optimizer folds constants, removes double negation, rewrites bool equality with true/false, recognizes self-equality and reuses identical pure operations within one block. It may discard an unused predicate, but checked conversions and owner effects that produced its operands remain observable. This does not make a read/compare pair atomic across threads or channels.

Both scalar and hosted Dis emitters lower bool.not to existing XORW with one, and bool.eq to an existing BEQW/materialization sequence. Direct int relations use BLTW/BEQW on normalized signed32 values, including NC64. No Dis opcode, frame/GC layout, native module ABI or heap sharing changes. Existing bool cells and lazy &&/|| continue to work.

The object backend emits standard AVM1 Not (0x12) and Equals2 (0x49). SWL 0.3.13 validates Not and preserves action bytes. Scalar FIR booleans therefore pass through FIR → SWO → linker → SWF; table-backed source programs still target Dis. No AS dynamic coercion or full AS compiler feature is implied.

## Build, inspect and run

From the repository root with matching built tools:

```sh
export TMPDIR="$PWD/build/tmp"
mkdir -p "$TMPDIR"
build/fir/fir1_source limbo --input fir/v1/examples/dis/typed-predicates.b build/predicates.fir -g build/predicates.json
build/fir/fir1 opt build/predicates.fir build/predicates.opt.fir
build/fir/fir1 dis build/predicates.fir build/predicates.dis
build/runtime/disvm --jit --quantum 1 --stats --inspect-words 0 2 build/predicates.dis
build/runtime/disvm --no-jit --quantum 1 --stats --inspect-words 0 2 build/predicates.dis
build/fir/fir1 object fir/v1/examples/dis/booleans.fir build/booleans.swo
build/swf-linker/swl link build/booleans.swf build/booleans.swo
```

The Limbo example returns 42 with validity flag 1. Repeat Dis emission/execution for predicates.opt.fir. Its local table must be released; runtime stats report tables_live=0 and keys_live=0. Inspect raw FIR for direct i32 predicates and bool.not/bool.eq, then optimized FIR for eliminated double negation. The scalar FIR example traces true (1 in FlashVM's current trace conversion); run it in a trace-capable SWF host to verify execution, rather than checking only that a movie opens.

Every nonblank line of both examples is commented. Installed SDK acceptance compiles the Limbo tutorial through both CLI and freshly linked C99 libraries, compares Dis bytes, then executes raw/optimized images with native and interpreter dispatch. The C99 source API remains serialized and blocking: an IDE should run it on its build worker and send copied results through its service channel.

Tests cover 216 loaded signed32 relation cases, bool truth tables, nested/lazy conditions, raw/optimized branch capture mapping, algebraic rewrites, checked producer failures, invalid source/API types and exact authored UTF-8 operator positions. Native counters establish host JIT execution; they do not certify ARM64 hardware or a complete compiler/IDE implementation.
