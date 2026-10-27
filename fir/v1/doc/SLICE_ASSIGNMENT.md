# Resized table and Unicode string assignments

[Русский](SLICE_ASSIGNMENT.ru.md) · [Relative positions](RELATIVE_SEQUENCES.md) ·
[Executable source](../examples/tables/splice.b) · [C storage API](../../../common/table/SPLICE.md)

The Coco/R source compiler, FIR verifier/optimizer, independent C reference
runner and native Dis backend implement these assignments in the bounded Limbo
big-table profile. Use FIR tools version 0.33 or later with a matching hosted
DisVM: module function signatures are checked before execution.

```limbo
# Replace [1,3), shift the tail and preserve the table owner.
t[1:3] = table of big {9};
# Insert a snapshot of the table's own original dense prefix.
t[1:1] = t;
# String bounds are explicit int rune positions, not UTF-8 bytes.
s[int 1:int -2] = "é";
# Replace one existing rune, with scalar validation.
s[int -1] = int 128512;
```

Both range bounds are currently required in this source profile. A nonnegative
end is exclusive; a negative end is inclusive, so `[0:-1]` covers the complete
sequence. Empty destination ranges insert, empty replacements delete, and tails
shift to fit. Table replacements use only the RHS dense numeric prefix, retaining
the destination's dictionary and absolute sparse keys. A growth collision with
an existing sparse key fails without modifying the destination.

The receiver and both bounds are evaluated once; relative bounds are normalized
before evaluating RHS. A RHS which changes the table's shape does not retarget a
negative destination: captured absolute bounds are checked again at commit.
Self replacement reads the old contents and publishes a complete new storage
layout. Allocation failure preserves the old destination. This is not a
transaction enclosing arbitrary RHS side effects: those effects are not rolled
back if the subsequent replacement fails.

Strings count Unicode scalars (runes), including combining marks separately.
Scalar writes accept explicit int values from U+0000 to U+10FFFF excluding
surrogates; out-of-range positions do not append. Slice replacements require a
string. A new independent string is bound to the assigned local, preserving old
aliases. ASCII-only replacements retain narrow storage where possible; wider
strings use rune storage and bulk copies. The entire output is initialized before
publication. Compound assignments to slice/scalar string targets are rejected.

FIR adds `table.range` and `string.range` capabilities: each retains a heap owner
plus two absolute exclusive bounds. `table.range`/`string.range` operations
capture normalized destinations; `string.char.range` captures one checked rune.
`table.splice` has no result and retains read/write/allocation/throw effects;
`string.splice` produces a fresh string. `rune.text` validates an int scalar and
builds a one-rune string. Copying a capability preserves all three cells; only
its first cell is a GC root. Table and string capabilities cannot be interchanged.
The Dis backend uses checked native module calls, without new upstream opcodes.

Debug events retain the authored opening bracket span before RHS. Tests compare
original/optimized FIR and actual interpreter/JIT execution with quantum 1 and
retired table/snapshot owners, plus an independent Unicode/sequence oracle.
Malformed C API types are rejected before either backend writes output.

This extends tables and strings, not general source arrays: resized native Dis
array assignment is still pending. Operation-owned mutex integration with VM
payload storage and remote projections is also pending. The current host serializes
individual C builtin operations on its VM owner thread; no cross-thread or
cross-request atomicity is claimed for the entire source expression. Regex slice
syntax and replacement captures remain separate work. None of these language
extensions is imported into Inferno.


## SDK tutorial / запуск примера

From the installed FIR package / из установленного каталога FIR:

```sh
./fir1_source limbo --input examples/tables/splice.b splice.fir
./fir1 opt splice.fir splice.opt.fir
./fir1 dis splice.opt.fir splice.dis
../disvm-runtime/disvm --jit --quantum 1 --inspect-words 0 4 splice.dis
```

Expected / ожидается: `DATA 0 42`, `DATA 8 0`, `DATA 16 1`, `DATA 24 0`.
The last two cells are execution flags; the failure arm is not executed.
Последние две ячейки — признаки исполнения; ветка ошибки не исполняется.
For in-process compilation / для встраивания:
[commented C99 client / комментированный клиент C99](../examples/dis/main.c).
