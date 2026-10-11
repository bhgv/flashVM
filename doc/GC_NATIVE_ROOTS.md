# Temporary native GC roots

`flashvm/gc_roots.h` provides a C89-compatible temporary root scope.

## Contract

1. Call `fvm_gc_roots_begin(context, &scope)` before manipulating unanchored object values.
2. Add each object-valued `FVM_Value` with `fvm_gc_roots_add(scope, &value)`.
3. While the scope is active, `fvm_gc_collect()` treats these objects as roots and follows their object properties, indexed entries, prototypes and metatables.
4. Close scopes in strict reverse order with `fvm_gc_roots_end(scope)`.

Values are copied into the scope; strings are **not** duplicated. This mechanism protects GC-managed objects, not arbitrary C buffers. Do not use a scope on another thread, or keep one after its context has been destroyed. The caller must keep the context alive until all scopes close.

## Limitations and next work

This is an explicit native-root facility, not a complete automatic root discipline. Native callbacks and their temporary arguments/results must still be audited. GC of strings, finalizer reentrancy, automatic safe-point scheduling, thread-local VM ownership, and the ActionScript `gc` object are not complete. No full-GC-completeness claim is made.

The executable regression test is `tests/gc_native_roots.c`.
