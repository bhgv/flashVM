# AVM1 automatic GC safe points

The AVM1 interpreter checks the allocation threshold at instruction boundaries,
currently once every 64 dispatched instructions (including the first). This
interval is a polling cadence, not a guarantee that a collection occurs.

Before the check, the interpreter has registered the operand stack, local
bindings and receiver (`this`) in its root chain. Collection is skipped while
native callbacks execute and when GC is suspended. The collector runs only
when the allocation threshold is reached. `full` performs a separate one-shot
collection and preserves the prior mode.

## Native integration

An object returned through the C API is not automatically pinned. Native
clients must pin it or place it in a temporary GC root scope if it must remain
alive across an AVM1 execution or collection. Pointers to reclaimed objects
must not be reused. Do not call the context concurrently from multiple threads.

## Test

`tests/gc_avm1_safe_point.c` allocates unreachable objects, runs an AVM1 End
action and verifies that the automatic safe point reclaims the objects while
preserving the global object.

## Remaining limitations

This does not complete the collector: native ownership, finalizer reentrancy,
script-defined `__gc`, value/string lifetimes and cross-context object transfer
still require further work and validation.
