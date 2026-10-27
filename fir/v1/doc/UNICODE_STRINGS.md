# Unicode string primitives — compiler 0.26

[Русский](UNICODE_STRINGS.ru.md) · [Commented tutorial](../examples/tables/unicode.b) ·
[Examples](../examples/tables/README.md) · [Lazy expressions](SHORT_CIRCUIT.md)

Our Coco/R Limbo big/table profile supports string length, character indexing and
half-open slicing. Types follow Inferno's string operations:

| Source | Input | Result |
| --- | --- | --- |
| `len text` | string | int Unicode-scalar count |
| `text[index]` | string, int | int Unicode scalar |
| `text[first:end]` | string, int/int | string, half-open slice |

Literal numbers in this profile are big, so bounds require `int 1`, an int local,
or another int expression such as `len text`. Table/key snapshot lengths stay big
and table indices retain their existing big/text/key rules. Implicit int/big
mixing remains rejected. Compiler 0.27 adds same-type int + - * / %; use
explicit casts where needed. This is a bounded profile, not complete Limbo/AS string coverage.

## Unicode and aliases

Positions count Unicode scalars, not UTF-8 bytes, UTF-16 code units or grapheme
clusters. Emoji above U+FFFF count once. `e` followed by U+0301 counts twice;
there is no normalization or grapheme segmentation. Empty strings have length
zero and permit `[int 0:int 0]`; indexing an empty string fails.

Indexing requires 0 <= index < length. Slicing requires 0 <= first <= end <=
length. Unused invalid indexes/slices still fail before and after optimization.
A lazy skipped operation does not execute. Slicing returns an immutable result
and preserves every source alias, including full, prefix, suffix and empty slices.
Nested slices can be used as table keys or converted with existing string/big APIs.
There is no zero-terminated-buffer assumption for a reference slice.

Public C99 IR constructors now receive the same strict UTF-8 invariant as textual
input: malformed live string constants fail verifier preflight. Lone continuation
bytes, overlong encodings, surrogates, values above U+10FFFF and truncation are
rejected before target output is written. This fixes an API validation gap; it is
FlashVM FIR code and is not an Inferno interpreter patch.

## FIR and execution

Three operation IDs append after the existing ones, preserving earlier IDs:

| FIR | Operand/result types | Effects |
| --- | --- | --- |
| `string.len %s` | string -> i32 | reads memory |
| `string.at %s, %i` | string / i32 -> i32 | reads memory, may throw |
| `string.slice %s, %range` | string / big.range -> string | reads memory, allocates, may throw |

Source int slice bounds widen exactly into the existing range representation.
The hosted Dis backend emits existing LENC (84), INDC (82) and SLICEC (113), with
correct frame-relative middle operands for indexes and slice ends. No new VM
opcode, Bigtable native helper or container tag is added. The current string path
uses the hosted backend and its existing Bigtable module/range-check setup;
this is not a standalone string-only runtime dependency removal.

Before SLICEC, MOVP retains the source into a distinct rooted destination. Thus
native in-place shortcuts cannot mutate the original string. Existing guarded
JIT paths or C fallback execute the instructions; --jit does not mean each slice
runs natively. String/frame roots remain separate from FlashVM's heap. The C99
reference runner independently decodes bounded UTF-8 and borrows immutable slice
bytes until invocation cleanup, rather than emulating Dis's byte/Rune layout.

## Diagnostics and limits

CLI and C99 source APIs share the Coco/R grammar and semantic actions.
Source events retain the exact len token or opening bracket, including UTF-8 prefixes;
receiver reads keep their own binding occurrences. Result/type
mismatches and invalid static bounds kinds preserve old FIR/sidecar destinations.
Native nil/bounds errors retain Inferno identities; the reference reports failure
without promising identical error wording. Full Dis SBL/debugger attachment and
GUI compiler panels remain separate work.

The existing source/token/value limits remain, including bounded string literals.
Tests cover ASCII, Latin/Cyrillic, non-BMP emoji, combining marks, escaped bytes,
empty/full/nested slices, aliases, numeric conversions of nonterminated reference
slices, key snapshots, eager errors, lazy skips, wrong types and forged C99 UTF-8.

## Run

From the repository root after building FIR and runtime:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/tables/unicode.b build/unicode.fir -g build/unicode.json
build/fir/fir1 check build/unicode.fir
build/fir/fir1 opt build/unicode.fir build/unicode-opt.fir
build/fir/fir1 dis build/unicode.fir build/unicode.dis
build/runtime/disvm --jit --quantum 1 --inspect-words 0 4 build/unicode.dis
build/runtime/disvm --no-jit --quantum 1 --inspect-words 0 4 build/unicode.dis
```

Value cells are `42, 0`, followed by executed flags `1, 0`. Relocated C99 clients
compile original source in process and verify original/optimized FIR on both VM
modes. Host execution is not physical ARM64 or full Inferno boot evidence.
