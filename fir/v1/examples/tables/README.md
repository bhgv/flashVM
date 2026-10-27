# Native hybrid-table tutorial

[Русский](README.ru.md) · [Limbo source](main.b) ·
[Reflection/cast contract](../../doc/TYPE_REFLECTION.md) ·
[Dis ownership contract](../../doc/TABLES_DIS.md)

main.b is a complete module for our Coco/R big-result subset. Each statement is
commented. It demonstrates dense/sparse numeric keys, string fields, aliases,
independent slices/key snapshots, retained snapshot keys, runtime typeof and
standard Limbo big/string cast syntax. Its result is exactly 42.

From the workspace root, with matching compiler 0.19/runtime 0.22 builds:

```sh
export TMPDIR="$PWD/build/tmp"
mkdir -p "$TMPDIR"
build/fir/fir1_source limbo --input fir/v1/examples/tables/main.b tables.fir -g tables.debug.json
build/fir/fir1 check tables.fir
build/fir/fir1 opt tables.fir tables.opt.fir
build/fir/fir1_table_run tables.opt.fir
build/fir/fir1 dis tables.opt.fir tables.dis
build/runtime/disvm --no-jit --quantum 1 --inspect-words 0 2 tables.dis
build/runtime/disvm --jit --quantum 1 --inspect-words 0 2 tables.dis
```

The optional C reference runner prints 42. Each Dis invocation captures DATA 0 42
and DATA 8 1; the latter marks that the trace executed. quantum=1 exercises GC
between instructions. This does not require the legacy compiler for program
emission. The host runtime build still uses its explicit bootstrap for native
module ABI generation and ordinary Limbo service modules.

compile.c is a C99 parser/Dis-emitter client; client.c embeds reference execution.
They use matching public headers/archives and stage output as documented in their
source comments. The installed SDK carries these sources and locally linked guides;
its acceptance test compiles/executes this module and relinks a fresh C99 client.

A snapshot key retains its independent owner after the local snapshot is replaced.
A string cast deliberately copies its value, so converting numeric key 7 to text
changes the namespace to "7". typeof does not perform that conversion. Decimal
casts preserve all signed-64 bits and reject malformed or overflowing text.

These files do not implement automatic channel any/proxy locals, general Limbo
modules, arbitrary table element types, SWF table generation.
AS conversion helpers remain separate from Limbo cast syntax. The debug JSON maps
to FIR source occurrences; full source-to-Dis SBL is still separate work.

[arithmetic.b](arithmetic.b) demonstrates signed wrap, negation and native big arithmetic on table reads. [Full contract](../../doc/BIG_ARITHMETIC.md).

[bitwise.b](bitwise.b) exercises signed division/remainder and full-width masks with runtime reads. [Contract](../../doc/BIG_BITWISE.md).

[Conditional owners tutorial / Пример с ветвлениями и владельцами](../../doc/TABLE_CFG.md) · [Исходник](decisions.b) · [Русский](../../doc/TABLE_CFG.ru.md).

[Checked shifts and int casts / Сдвиги и int-cast](../../doc/BIG_SHIFTS.md) · [Исходник](shifts.b) · [Русский](../../doc/BIG_SHIFTS.ru.md).

## Exact division and remainder

[division.b](division.b) reads every operand from a mutable table: MIN/-1 wraps
to MIN with zero remainder; -17/5 truncates to -3 and leaves remainder -2. The
selected arm reconstructs the dividend and returns exactly 42. Its other arm
divides by zero and must not execute. Each important source line is commented.
The installed SDK acceptance runs original and optimized FIR in the reference
runner, interpreter and JIT, with collection permitted at every instruction.
[Big arithmetic contract](../../doc/BIG_ARITHMETIC.md).

## Presence, deletion and total count

[mutation.b](mutation.b) is a line-commented tutorial for the compiler's new
countof/in/delete operations. It stores a present zero, aliases one owner, takes
an immutable snapshot, deletes through that snapshot and repeats the deletion.
It returns 42 while len observes a numeric hole and countof still counts both
remaining records. The unselected arm deliberately loads a missing value.
[Complete syntax, effects, ABI and run commands](../../doc/TABLE_MUTATION.md).
The relocated SDK tests original/optimized FIR through fresh C99 API clients and
both interpreter/JIT modes, checking values and branch execution flags.

## Terminal lexical arms

[arms.b](arms.b) returns 42 through either branch. It demonstrates initializer
lookup before shadowing, deferred typed initialization, local table aliases and
outer owner reassignment. Both branches terminate; no SSA join is implied.
[Step-by-step semantics and run commands](../../doc/TERMINAL_ARMS.md).

## Comparison values

[comparisons.b](comparisons.b) returns 42 using exact table-loaded integers above
2^53, stored bool comparisons, logical negation, typed int comparison and a local
terminal arm. Compare the live original and optimized captures in both VM modes.
[Detailed semantics, precedence and commands](../../doc/COMPARISON_VALUES.md).

## Lazy logical expressions

[short-circuit.b](short-circuit.b) returns 42 while guarding an absent-key read
and skipping a deleting RHS. No temporary table implements the logical join;
the compiler uses checked scalar frame cells and ordinary Dis MOVW.
[Semantics, FIR cells and commands](../../doc/SHORT_CIRCUIT.md).

## Captured compound updates

[compound.b](compound.b) demonstrates all ten compound operators on a shared
table alias, a captured snapshot key and a big local, returning 42. Shift counts
are explicit int. Locations and old values are captured before RHS evaluation;
the table remains exclusively owned until the store, without transaction rollback. [Contract and commands](../../doc/COMPOUND_UPDATES.md).

## Unicode string primitives

[unicode.b](unicode.b) checks seven Unicode scalars, a slice containing emoji and
source alias preservation, then parses a numeric slice to return 42. String lengths
and indexes are int; table/key lengths stay big. Combining marks count separately.
[Detailed semantics and commands](../../doc/UNICODE_STRINGS.md).

## Int arithmetic and computed string bounds

[int-arithmetic.b](int-arithmetic.b) computes a Unicode window using all five int
arithmetic operators and parses the resulting slice as 42. Every result wraps
to signed int32 on the NC64 target; literals require explicit int in this profile.
[Semantics and commands](../../doc/INT_ARITHMETIC.md).

## Status labels and exact int masks (0.28)

[text-bits.b](text-bits.b) builds an editor-style Unicode status label. It applies
three typed int compound bit operations, converts 42 explicitly to text, appends
an emoji and verifies that the old alias still reads `line 42`. String equality
and lazy bool expressions guard a scalar-indexed slice returning 42. The false
arm returns 0. Expected trace cells are 42/0, flags 1/0. Every step is commented.

[Detailed guide and commands](../../doc/TEXT_BITS.md) explains allocation effects,
frame roots, exact int narrowing, rejected mixtures and current SWF limits.
Installed SDK checks compile this tutorial through freshly linked C99 APIs as
well as CLI, before/after optimization, in interpreter and JIT modes.

## Checked int shifts and packed fields (0.29)

[int-shifts.b](int-shifts.b) reads the exact packed word 0x8000002a from a big
table, explicitly narrows it to int, extracts its sign with arithmetic >>31 and
its low-byte payload with a mask. Compound left/right shifts round-trip 42. A
lazy terminal decision keeps an invalid count 32 unexecuted. Every step is
commented; expected trace cells are 42/0 and execution flags 1/0.

[Detailed guide](../../doc/INT_SHIFTS.md) gives commands and explains width, eager
count errors, FIR effects, eight-instruction Dis expansion and current limits.
Installed SDK acceptance uses freshly linked source/FIR C99 clients, original
and optimized IR, interpreter and native JIT, with one-instruction GC quanta.

## Ordered text and parsed editor fields (0.30)

[text-values.b](text-values.b) snapshots a decimal settings key, deletes the
original and converts the retained key to int. A Unicode scalar slice yields
42 independently. Filename comparison demonstrates lexicographic rather than
natural sorting. A lazy bool condition checks the result and unchanged key kind.
Each step is commented. Expected traces: 42/0; execution flags: 1/0.

[Detailed guide and commands](../../doc/TEXT_VALUES.md) explains all six string
relations, strict decimal big parsing before signed32 narrowing, ownership,
errors and limits. Relocated SDK acceptance relinks fresh source/FIR C99 clients
and executes original/optimized IR in interpreter and JIT modes.

## Hybrid constructors and editor-style settings (0.31)

[constructors.b](constructors.b) combines two positional entries, named scalar
settings and explicit text/numeric keys. It verifies a numeric-prefix length of
two and six total keys. A numeric slice stays independent when a shared table
alias mutates the original. The source comments explain every field and step.
Expected trace cells: 42/0; execution flags: 1/0. Settings are example data, not
a change to the live IDE's preferences.

[The detailed guide](../../doc/TABLE_LITERALS.md) contains commands, collision
examples, key-before-RHS evaluation, type/limit rules and parser/service ownership.
Installed SDK acceptance relinks fresh C99 source/FIR clients and executes the
original/optimized tutorial in interpreter and JIT modes.

## Relative positions / Отрицательные позиции (0.32)

[relative.b](relative.b) reads the last dense-prefix elements, copies `[0:-1]`,
mutates the source and verifies copy independence. Unicode negative bounds
extract `42` from `Ж42😀`. Sparse/string keys do not affect negative numeric
positions. Every step is commented; expected trace cells: 42/0, flags: 1/0.

Пример читает хвост плотного префикса и проверяет независимость копии после
изменения источника. Отрицательная правая граница включается; положительная
остаётся исключающей. Позиции Unicode считаются скалярами.

[EN guide and commands](../../doc/RELATIVE_SEQUENCES.md) /
[RU: руководство и команды](../../doc/RELATIVE_SEQUENCES.ru.md).
Installed SDK checks use fresh C99 source/FIR clients, original/optimized IR
and both actual VM modes. The native array tutorial is linked from the guide.


[Resized assignments / присваивание срезам](../../doc/SLICE_ASSIGNMENT.md): `splice.b`, expected / результат `42`.


[Retained table assignments](../../doc/TABLE_LEASES.md) cover destination capture, RHS and store/splice; see the commented [leases.b](leases.b).


[ownership-flow.fir](ownership-flow.fir) demonstrates recursive aliases and a shared branch successor, returning 42. [Ownership-flow contract](../../doc/OWNERSHIP_FLOW.md).


[While and table for-in](../../doc/LOOPS.md) — executable loops, mutable big locals and a commented tutorial.


[Loop if/else, break, continue and early return](../../doc/LOOP_CONTROL.md).


[Counted for and postfix big updates](../../doc/COUNTED_FOR.md).


[Typed loop tutorial](typed-loops.b): int counters, bool stop flags, Unicode runes and independent key snapshots. [Guide](../../doc/TYPED_LOOPS.md).


[Retained-loop tutorial](retained-loops.b): reference rebinding, stable aliases and captured traversal. [Guide](../../doc/RETAINED_LOOPS.md).


[Function-flow tutorial](function-flow.b): conditional updates, lexical blocks, early guards and nested loop transfers. [Guide](../../doc/FUNCTION_FLOW.md).


[Join-initialization tutorial](join-initialization.b): initialize scalar/reference declarations in both arms, then use them safely. [Guide](../../doc/JOIN_INITIALIZATION.md).

[First initialization inside loops — 0.43](../../doc/LOOP_INITIALIZATION.md): typed outer declarations, safe body reads and conservative exit facts. [Commented tutorial](loop-initialization.b).

[Deferred for steps — 0.44](../../doc/DEFERRED_STEP.md): initialization proof over every incoming path, including continue. [Tutorial](deferred-step.b).
