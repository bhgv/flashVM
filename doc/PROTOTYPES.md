# Prototype API (implemented subset)

The public C89 header `flashvm/value.h` exports:

- `fvm_object_set_prototype`: link or clear an object's prototype; reject cycles
  and references across contexts.
- `fvm_object_get_prototype`: return the borrowed prototype pointer, or NULL.
- `fvm_object_has_own`: distinguish own properties from inherited properties.
- `fvm_object_get`: search own properties first, then follow prototypes.
- `fvm_object_set`: always write an own property (shadow inherited properties).

Objects are context-owned until context destruction. The API is not thread-safe.
Prototype support does not yet implement `__proto__`, `__resolve`, property
attributes, accessor methods, AVM1 constructor behavior, or script-defined
functions. C89 host code can use these functions now; AVM1 `GetMember` uses the
same inherited lookup.
