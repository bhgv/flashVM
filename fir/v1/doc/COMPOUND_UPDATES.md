# Captured compound updates — compiler 0.25

[Русский](COMPOUND_UPDATES.ru.md) · [Commented source](../examples/tables/compound.b) ·
[Examples](../examples/tables/README.md) · [Lazy expressions](SHORT_CIRCUIT.md)

The Coco/R Limbo big/table profile accepts statement forms `+=`, `-=`, `*=`, `/=`,
`%=`, `&=`, `|=`, `^=`, `<<=` and `>>=` on initialized big locals and captured table
entries. Dotted names, numeric/text indices and immutable snapshot keys all work.
These operator spellings were checked against Inferno's compound operation cases;
the hybrid-table capture order below is an explicit FlashVM extension contract.

```limbo
answer := big 40;
answer += 2;
entries[keys[0]] ^= big 3;
entries.answer <<= int 1;
```

## Types and arithmetic

Arithmetic/bitwise forms require old big plus RHS big. Shift forms require old
big plus explicit RHS int; counts must be 0..63. Inferred numeric literals in this
profile are big, so `x <<= 1` is a static error; use `x <<= int 1`.
The separate homogeneous int and AS profiles retain their previous operator
coverage. This does not add string concatenation, bool compound operations,
exponent assignment, increments/decrements or assignment-valued expressions.

Big addition/subtraction/multiplication/bitwise operations wrap modulo 2^64.
Division truncates toward zero; MIN/-1 wraps to MIN and MIN%-1 is zero. Zero
raises. Right shift is arithmetic. There is no conversion through binary64.
Arithmetic retains the existing operations and types. Automatic ownership now adds
FIR table.lock/unlock and checked Bigtable imports; Dis opcodes/file tags stay unchanged.

## Capture and publication order

1. Resolve the local binding or table owner and key once, in authored order.
2. For a compound update, capture the old big value before evaluating the RHS.
3. Evaluate the RHS once, including its ordinary calls/mutations/lazy CFG.
4. Compute the new big value and perform the final assignment/store only on success.

Plain `=` still performs no old-value read and can insert a missing table entry
or initialize a typed local. Compound assignment requires an initialized local
or present entry. A missing old entry raises before a bad RHS conversion or zero
division. For example, `t.missing += 1/0` reports missing key.

A captured table location is an owner/key capability, not a payload address.
`t.answer += big (delete t.answer) + 1` reads the old value first, removes the key
in the RHS, then reinserts old+2. Receivers and side-effecting keys are not retried
or reevaluated. Existing checked root maps retain owners/key snapshots across
RHS allocation and one-instruction scheduler/GC slices. Runtime errors do not
execute the final store; prior independent RHS effects are not rolled back.

Table destinations now retain [exclusive local ownership](TABLE_LEASES.md)
across key capture, old-value reads, RHS evaluation and the final store. Competing
payload operations cannot interleave in that scope. Already completed effects
are not rolled back. Remote compare/exchange, multi-object transactions and
general projection update protocols remain separate work.

## FIR, optimization and source origins

Lowering reuses checked table.index/load/store, typed arithmetic and shifts.
Locals update their current SSA binding. Table receiver/key and old value are
captured before RHS instructions. Existing MAY_THROW/memory flags keep failures
and mutations in order; optimization cannot duplicate a deleting key expression.
A lazy RHS preserves its own branch/merge cells before the final compound store.

Arithmetic/shift events retain the exact authored compound-operator bytes,
including three-byte <<= and >>= after UTF-8 prefixes. Local access records keep
the identifier's binding owner. Syntax/type/resource failures preserve previous
FIR/debug destinations. CLI and C99 API share this grammar and implementation.
The extension stays in FlashVM; no hybrid-table syntax is sent to Inferno.

## Run and check

After building FIR and runtime, from the repository root:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/tables/compound.b build/compound.fir -g build/compound.json
build/fir/fir1 check build/compound.fir
build/fir/fir1 opt build/compound.fir build/compound-opt.fir
build/fir/fir1 dis build/compound.fir build/compound.dis
build/runtime/disvm --jit --quantum 1 --inspect-words 0 4 build/compound.dis
build/runtime/disvm --no-jit --quantum 1 --inspect-words 0 4 build/compound.dis
```

Values are `42, 0`, followed by executed flags `1, 0`. Fresh relocated C99 compiler
and reference clients check original/optimized tutorial variants and both actual
VM modes. Tests cover all ten operators on locals/numeric/text/snapshot locations,
signed extrema, aliases, side-effecting keys/RHS, lazy RHS, eager error priority,
static rejection and exact UTF-8 source spans. Physical ARM64 and complete
IDE compiler/debugger integration remain separate acceptance work.

Compiler 0.27 / Компилятор 0.27: [int + - * / %, += -= *= /= %=](INT_ARITHMETIC.md),
[русское руководство](INT_ARITHMETIC.ru.md). Int bitwise/shift operations remain unsupported.

Compiler 0.28: [string concatenation/equality and int bit operations](TEXT_BITS.md).

Compiler 0.29: [checked int shifts and compound updates](INT_SHIFTS.md).

Compiler 0.31 / Компилятор 0.31: [hybrid constructors](TABLE_LITERALS.md), [гибридные конструкторы](TABLE_LITERALS.ru.md).


[Retained assignment ownership / Удержание на всё присваивание](TABLE_LEASES.md) supersedes the earlier per-primitive ownership boundary.
