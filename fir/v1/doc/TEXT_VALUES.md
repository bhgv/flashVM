# Ordered Unicode strings and checked int casts — compiler 0.30

[Русский](TEXT_VALUES.ru.md) · [Commented tutorial](../examples/tables/text-values.b) ·
[Examples README](../examples/tables/README.md) · [Text expressions](TEXT_BITS.md) ·
[Comparison values](COMPARISON_VALUES.md) · [Int arithmetic](INT_ARITHMETIC.md)

The Coco/R Limbo big/table profile now accepts all six same-type string relations
and explicit int conversion of a string or an immutable snapshot key:

```limbo
ordered := "file10" < "file2";
line := int digits;
key_number := int keys[0];
valid := ordered && (line == key_number);
```

## Ordering contract

`<`, `>`, `<=`, `>=`, `==` and `!=` return bool. Operands evaluate once, left
before right, including relations implemented by swapping operands or negating
a normalized result. A shorter equal prefix sorts first. Empty strings compare
normally. Comparing a string to a number, table or bool remains a static error.

Ordering uses Unicode scalar values, matching Inferno's stringcmp/BLTC contract.
There is no locale, normalization, case folding, UTF-16 code-unit ordering or
natural numeric sorting: `"file10" < "file2"`, `"A" < "a"` and `"é" != "é"`.
U+E000 sorts before U+1F600 despite the opposite UTF-16 code-unit ordering.
Concatenations, aliases, reflected type names and half-open slices are supported.
The separate homogeneous int and AS numeric source profiles are unchanged.

## Int conversion contract

For text and text snapshot keys, `int` composes the existing checked `big`
conversion with total modulo-2^32 signed narrowing. Parsing accepts the complete
nonempty ASCII decimal sequence, an optional leading +/-, and leading zeros.
The parsed value must fit signed64 first. Whitespace, suffixes, non-ASCII digits,
fractions, empty/sign-only text and big overflow raise `invalid big conversion`.

| Text | Int result |
| --- | --- |
| `"+00042"` | 42 |
| `"2147483648"` | -2147483648 |
| `"4294967295"` | -1 |
| `"4294967296"` | 0 |
| `"9223372036854775808"` | error before narrowing |

Numeric snapshot keys use their captured exact big value before narrowing.
Text snapshot keys use their copied text. Conversion never changes the key kind
or reacquires the mutable table: snapshots remain valid after source deletion.
Int/big/bool conversions retain their earlier behavior. Tables/key collections
cannot be converted directly; select an individual key first.

This is the bounded profile's strict checked conversion contract. It does not
replace Inferno's native CVTCW/strtol behavior or claim full Limbo lexical-cast
compatibility. Raw VM conversion operations and the existing runtime ABI are
unchanged. An unused failing parse still raises before and after optimization;
a genuinely skipped lazy/terminal arm does not execute its conversion.

## FIR, optimizer and ownership

One append-only operation, `string.lt`, consumes string/string and returns bool
with READS_MEMORY. Existing string.eq handles equality. Source inversion/swapping
implements the remaining relations without reevaluating either expression.
Constant ordering folds using canonical UTF-8 unsigned-byte lexicographic order,
which preserves scalar order. Dynamic comparisons emit existing Dis BLTC with
rooted operands; AMD64/ARM64 native kernels and C fallbacks remain authoritative.
No opcode, builtin helper ABI or container representation is added.

The reference comparison uses memcmp over the common byte extent, then compares
lengths; it never assumes a slice ends in NUL. Text/key-to-int source lowering
emits value.big followed by value.int. The latter's FIR signature remains
int/big/bool → int: a forged direct string operand is rejected by the C99/textual
verifier. Parsing keeps its conservative effects, so narrowing cannot erase an
unused error. Both emitted cast operations retain the authored int token span
in debug JSON after a Unicode prefix.

Live ordered-string/text-key casts require the reference runner or hosted Dis;
SWF rejects unsupported live operations before replacing its output. This prepares
IDE source services; full IDE Build/Run/Debug panels and debugger attachment are
separate work. These compiler extensions stay FlashVM-only.

## Run and expected result

From the repository root after building FIR and the native runtime:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/tables/text-values.b build/values.fir -g build/values.json
build/fir/fir1 check build/values.fir
build/fir/fir1 opt build/values.fir build/values-opt.fir
build/fir/fir1 dis build/values-opt.fir build/values.dis
build/runtime/disvm --jit --quantum 1 --inspect-words 0 4 build/values.dis
build/runtime/disvm --no-jit --quantum 1 --inspect-words 0 4 build/values.dis
```

Expected trace cells: 42, 0; executed flags: 1, 0. Relocated SDK checks relink
fresh source/FIR C99 clients and execute the original/optimized tutorial in both
VM modes. Existing ARM64 kernels are checked by instruction emulation; physical
ARM64 acceptance remains separate.
