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
