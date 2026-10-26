# Acyclic table control flow — FIR 0.19 / runtime 0.22

[Русский](TABLE_CFG.ru.md) · [Fully commented Limbo](../examples/tables/decisions.b) ·
[Typed source](TABLES.md) · [Ownership ABI](TABLES_DIS.md) · [Big relations](BIG_DECISIONS.md)

Coco/R big-result source decisions can now carry live table, snapshot, retained
key, string and location owners into both arms. Native calls and conversions
execute only on the selected path. Existing terminal source syntax is unchanged:
a declaration/assignment prelude, then return or nested if/else with every arm
terminating. This closes the previous gap where valid source/FIR decisions with
tables could not be emitted as Dis or executed by the independent reference runner.

## Graph and ownership contract

The hosted Dis backend and C reference runner require a verified DAG. New public
`fir_acyclic(module, error, capacity)` verifies the borrowed module, deduplicates
CFG edges and performs a bounded topological walk. It changes no module/parser
state, allocates no heap, and admits backward physical branch targets when the
logical graph has no cycle. Cycles fail preflight before output or owner allocation.
The general FIR verifier and pointer-free scalar Dis backend still permit cycles;
the existing runtime instruction budget remains their execution boundary.

A DAG executes each SSA-producing instruction at most once per invocation, keeping
the reference owner registries bounded by FIR_LIMIT. Native frame descriptors
retain the same precise maps for all possible values. Unselected owner cells
remain initialized H roots; scalar bools, indices/ranges and raw prepared-frame
addresses are unmapped. Module imports load once before entry instructions.
No pointer is passed between VM heaps or serialized onto the network.

Manual FIR can also join paths through a common dominated table owner/location:
table mutations remain ordinary sequential side effects, not phi values or an
atomic remote read-modify-write. Duplicate IF targets are one CFG edge. A source
assignment/join language feature, phi, source loops and cyclic table execution
are not introduced here.

## Instruction PCs and errors

A private first emission measures every block's final instruction PC, including
native frame/argument/call expansions and internal bool materialization. A fresh
second stream resolves forward/backward IF/JMP targets. Encoded byte widths do
not affect instruction counts; the two counts must agree before publication.
No placeholder code or partial first pass reaches caller output. Existing
HASLDT imports and opcodes remain unchanged; IF compares normalized bool to
immediate one before selecting an existing JMP target.

Static trace cells and executed flags keep their established layout: N values,
then N flags. Count live traces again after optimization. The reference output
is chronological and staged until successful return. Selected missing-key,
conversion and zero errors publish no reference trace or native DATA inspection.
An unselected failing arm does not execute. Final output I/O failure can still
leave a prefix; the CLI stages destination replacement.

## Run the example

From the repository root after independent FIR/runtime builds:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/tables/decisions.b build/table-decision.fir
build/fir/fir1_table_run build/table-decision.fir
build/fir/fir1 dis build/table-decision.fir build/table-decision.dis
build/runtime/disvm --jit --quantum 1 --inspect-words 0 6 build/table-decision.dis
build/runtime/disvm --no-jit --quantum 1 --inspect-words 0 6 build/table-decision.dis
```

Reference result: `42`. Native cells: `42, 0, 0, 1, 0, 0`: the first of three
static returns executed. Both relocated SDKs relink public compiler/reference
clients and run this example; no bootstrap compiler is involved.

Acceptance covers both source outcomes, nested conditions with retained key
snapshots and casts, unselected new owners/errors, selected faults, backward
shared exits, duplicate CFG edges, and branches across 8191 native instructions.
Both dispatch modes collect between instructions and retire all table/key owners.
UBSan covers the DAG verifier, compiler and reference execution. ARM64 instruction
emulation remains distinct from physical ARM64 acceptance.

[FIR 0.20: typed big shifts and int / Типизированные сдвиги и int](BIG_SHIFTS.md) · [Русский](BIG_SHIFTS.ru.md).
