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
modules, arbitrary table element types, table CFG/loops or SWF table generation.
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
