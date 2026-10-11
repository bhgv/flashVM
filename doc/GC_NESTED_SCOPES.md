# Nested native GC root scopes

Native extensions can open multiple temporary root scopes. Scopes form a strict last-in, first-out stack. Closing an outer scope while an inner scope remains open returns `FVM_INVALID_ARGUMENT` and leaves both scopes intact.

Each scope copies `FVM_Value` entries. Object entries protect the referenced objects until that scope is closed; this does not transfer ownership of the objects or make their raw pointers safe after collection.

The regression test `tests/gc_nested_scopes.c` checks nesting, invalid closure order, survival through collection, and reclamation after scope closure. This is **not** a substitute for full AVM1 and native-call root integration, which remains incomplete.
