# Explicit interface for a future native C module; no implementation is supplied here.
Counter: module
{
    # Runtime lookup spelling retained as an authored constant, not resolved by the catalog.
    PATH: con "$Counter";

    # Heap layout and its pointer map must be derived by a later matching Dis ABI pass.
    State: adt
    {
        value: int;
        changed: chan of int;
    };

    # Construct a state whose ownership follows the Dis module/heap conventions.
    create: fn(initial: int): ref State;

    # Update the state; C argument/result frame layout is not emitted by this catalog.
    add: fn(state: ref State, delta: int): int;

    # Expose a typed event channel without importing Inferno UI libraries.
    events: fn(state: ref State): chan of int;
};
