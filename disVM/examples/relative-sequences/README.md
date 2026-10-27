# Native relative array/string positions

[Русский](README.ru.md) · [Source](main.b) ·
[Semantics](../../../fir/v1/doc/RELATIVE_SEQUENCES.md)

Build the existing runtime target; it compiles `relative-test.dis`. Run from the
repository root:

```sh
build/runtime/disvm --jit --quantum 1 --stats build/runtime/relative-test.dis
build/runtime/disvm --no-jit --quantum 1 --stats build/runtime/relative-test.dis
```

The module prints PASS after checking byte/int/big/real element access, negative
writes and slice-assignment offsets. `[0:-1]` copies independent storage; mutating
a full or partial copy leaves the source unchanged. Mapped record strings survive
source retirement, while ref ADT elements retain object identity. Unicode indexes
count scalars; negative character writes preserve old string aliases through COW.
Empty copies are accepted. Every phase is commented in the source.

This native-array fixture uses the retained reference Limbo compiler, not the
bounded Coco/R frontend. Its table/string counterparts are compiled through our
own source/FIR services. Relative array slices currently clone the full source
before narrowing; positive-only slices retain the original shared-view behavior.
Physical multi-thread payload sharing still needs the separately documented
owner-token guards; the current host serializes C operations on its VM owner.
