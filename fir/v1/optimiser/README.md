# FIR-to-FIR peephole

[Русский](README.ru.md). Implementation: [peephole.c](peephole.c).

`fir1 opt INPUT OUTPUT` verifies before and after rewriting. It repeatedly:

1. Folds i32 add/sub/mul and i32 comparisons of known integer constants.
2. Replaces i32 x+0 and x-0 with x.
3. Propagates copy operands without introducing re-evaluation.
4. Removes unused constants and typed arithmetic/comparisons.

Unsigned compiler arithmetic defines modulo-2^32 folding without C signed
integer overflow. Uses are symbol indices, not text replacement. Iteration
continues until no change. The pass also folds constant if, removes unreachable blocks/cycles and reuses
identical ordered arithmetic expressions within one block. It does not move
loads, merge blocks or inline calls.
Dynamic addition/conversion, trace and channel effects are preserved even when
the result is unused. f64 identities and constant folding are deliberately
absent, preserving negative zero and runtime FP behaviour.

```text
// Before
%a: i32 = const.i32 6
%b: i32 = const.i32 7
%c: i32 = i32.mul.wrap %a, %b
trace %c
// After (unused a/b removed)
%c: i32 = const.i32 42
trace %c
```

Text comments are not retained verbatim; input-line notes remain. This is an
IR pass, independent of a future AVM1 instruction peephole. See
[format/effects](../doc/FORMAT.md) and [tests](../doc/TESTING.md).

Local CMake entry: configure this directory with the same COCOR_ROOT and
COCOR_EXECUTABLE flags; tool executables are under the build directory’s `v1/`.

## 0.2.1

`effects.c` defines six effect flags; unknown operations receive all flags.
DCE consults that table instead of enum ordering. A constant `if` becomes `jump`;
unreachable blocks, including cycles, are removed and labels remapped. Effects
are removed only on a proven unexecuted path. CSE reuses identical ordered
arithmetic in one block, without constant-folding f64. Dynamic conversions,
channels and trace remain even when their results are unused.

[C API and embedding](../compiler/API.md).
