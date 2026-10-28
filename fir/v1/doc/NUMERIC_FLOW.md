# Numeric cells and structured Limbo flow — FIR 0.57 / source CLI 0.24

[Русский](NUMERIC_FLOW.ru.md) · [Coverage](COVERAGE.md) · [Grouped expressions](GROUPED_CONDITIONS.md) · [Int tutorial](../examples/dis/numeric-flow-int.b) · [Real tutorial](../examples/dis/numeric-flow-real.b) · [FIR loop](../examples/dis/numeric-cells.fir) · [Signed-zero snapshots](../examples/dis/real-cells.fir)

Homogeneous Limbo int/real functions now admit ordinary structured statements:
nonterminal if/else, optional else, else-if, while, innermost break/continue and
return from a nested block or loop. A source function still uses the existing
bounded value() entry and trace harness. This is useful scalar control flow,
not a general module/function-call ABI or a complete Limbo compiler.

## Source flow and static facts

Initialized numeric locals retain their exact type. Before a branch or loop,
active values are promoted into private dominating cells. Assignments update
those cells; later reads produce fresh immutable SSA snapshots. Both open if
arms rejoin through the same cell. A returning arm contributes no fallthrough
edge. An omitted else preserves the incoming state, and else-if uses the same
bounded structured machinery.

While evaluates its bool condition again on every backedge. Continue targets the
innermost test; break targets the innermost exit. Body-local declarations and
aliases retire at lexical close and cannot escape through a post-loop read.
The source proof conservatively treats loops as possibly empty, even for a
constant-true test; a body return alone never proves a total function return. Every reachable function exit must return the declared numeric kind;
unreachable statements after a transfer reject. Numeric declarations still need
initializers. Numeric for, named transfers, bool locals, mixed int/real casts and
source calls remain outside this profile. The table/big profile keeps its existing
for/while, conversions and retained-owner cell behavior.

Numeric and table statement repetitions have separate Coco/R FIRST sets. In
particular, unsupported numeric for syntax produces a bounded compiler diagnostic
rather than entering a repetition whose error arm cannot consume the token.
All new syntax is parsed by Coco/R; no second source scanner is introduced.

## FIR cell contract

Two types and six opcodes append to the existing enum catalogs without renumbering
old entries. Textual FIR remains version 1.0; callers must use matching tools and
relink C99 libraries/headers. No stable binary ABI for compiler structures is promised.

| Type | Constructor | Store | Load | Initial payload |
| --- | --- | --- | --- | --- |
| i32.slot | i32.local | i32.store cell, i32 | i32.load cell | signed32 zero |
| f64.slot | f64.local | f64.store cell, f64 | f64.load cell | binary64 positive zero |

Constructors define private frame cells; store has no SSA result, load defines an
immutable payload value. A cell handle cannot be copied, traced, sent through a
channel or converted to another type. Its constructor must dominate every use.
Raw FIR permits a load immediately after construction, yielding zero; source
initialized-local rules remain stricter. Different cells cannot alias. A later
store does not change a previous load snapshot. A constructor executed again on
a loop iteration initializes its storage again.

Constructor/store effects are FIR_WRITES_MEMORY; load is FIR_READS_MEMORY. No
heap allocation or owner retention occurs. The optimizer forwards exact same-block
scalar loads and removes overwritten writes under the existing effect barriers,
resetting facts at block edges, unknown writes, suspension and relevant errors.
It never forwards a source value across a backedge or turns an effectful producer
failure into a successful program. Verifier signatures, arity, dominance and
nonescape checks run before backend publication.

## Backends, GC and debug

Scalar and hosted Dis emit existing MOVW/MOVF/MOVL operations into ordinary raw
frame storage. Even alongside a hosted table, numeric cells receive no pointer-map
bits and keep NaN/signed-zero payloads out of GC roots. No new Dis opcode, native
frame ABI or shared pointer is introduced. Aggregate native counters prove actual
selected dispatch, not physical foreign-CPU acceptance or native coverage of every
possible operand path.

SWO emission uses object-local variables and standard AVM1 GetVariable/SetVariable,
with the existing signed branch fixups. SWL creates the ordinary SWF8 container.
No new container tag or relocation ABI is required. The independent C99 reference
executes its own numeric payload fields. Checked integer division/shifts retain
their existing SWF capability limits; successful int/real control flow does not
claim all such exceptions are now implemented by SWF.

FIRDBG1 preserves original UTF-8 spans for while, break, continue and return,
including nested paths. Synthetic cell construction is attributed to the controlling
if/while; legacy table-operation event spelling is retained for these typed
operations. This is source/unoptimized-FIR provenance, not final Dis/SWF/JIT PC
maps. API calls remain serialized and blocking; the host owns streams and returns
copied build results over IDE service channels.

## Run the commented examples

Every nonblank line in all four tutorials has an explanatory comment.

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/dis/numeric-flow-real.b build/flow.fir -g build/flow.json
build/fir/fir1 opt build/flow.fir build/flow.opt.fir
build/fir/fir1 dis build/flow.fir build/flow.dis
build/runtime/disvm --jit --quantum 1 --stats --inspect-words 0 2 build/flow.dis
build/runtime/disvm --no-jit --quantum 1 --stats --inspect-words 0 2 build/flow.dis
build/fir/fir1 object build/flow.fir build/flow.swo
build/linker/swl link build/flow.swf build/flow.swo
```

The real example returns exactly 42.5: capture bits 4631178160564600832 and validity
flag 1. The int example skips index 3, breaks at 9, accumulates 33 and rejoins to
return 42. It has two source returns: raw captures -7/42 have values 0/42 and flags
0/1, so use --inspect-words 0 4. Count surviving trace sites after optimization.
For numeric-cells.fir skip source compilation: it returns 42 after six iterations.
real-cells.fir captures +0/-0; four inspected words are 0, -9223372036854775808, 1, 1.
SWF numeric trace is display text; exact signed-zero payloads are checked on Dis.

Acceptance covers raw/optimized CLI and public C99 APIs, loop-bound matrices,
nonterminal joins, nested loops, shadowing, early returns, strict rejection and
staged prior-output preservation. Scalar and hosted GC cases include signed32
minimum, signed zero and NaN. Located source origins are checked after a Unicode
prefix. Relocated SDKs relink a fresh C99 client, compare CLI/library Dis and run
the real loop tutorial in interpreter/JIT. Full source languages, AS3 ABC, general
Dis linking/calls, self-hosted IDE compilation and Build/Run/Debug panels remain
separate milestones.
