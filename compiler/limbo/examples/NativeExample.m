# Interface-only example: generated scaffolds do not invent the native Dis ABI.


# Counter describes typed state and entry points for a future external implementation.
Counter: module                                    # Lookup/type identity supplied to the host resolver.
{                                                   # Begin the public module header.
    PATH: con "$Counter";                           # Authored loader spelling; the catalog does not resolve it.


    # State is a managed ADT; its physical layout requires a matching Dis ABI adapter.
    State: adt                                      # Name retained by type/signature metadata.
    {                                               # Begin the managed fields in declared order.
        value: int;                                 # Current signed 32-bit counter value.
        changed: chan of int;                       # Typed notification channel; send/receive may block.
    };                                              # Complete the ADT, including its required semicolon.


    # Allocate and initialize a counter state.
    # Parameters: initial is the starting i32 value. Returns a managed State reference.
    # A native implementation must follow the selected Dis heap/root conventions.
    create: fn(initial: int): ref State;             # Composite marshaling is not supplied by this header.


    # Change an existing state and report its resulting counter value.
    # Parameters: state is the managed state reference; delta is the signed increment.
    # Returns an int. Native frame layout and notification policy require implementation.
    add: fn(state: ref State, delta: int): int;       # No guessed C structure layout is part of this declaration.


    # Obtain the typed notification capability associated with a state.
    # Parameters: state is the managed state reference. Returns its chan of int.
    # The bridge must validate/retain capabilities instead of transferring native pointers.
    events: fn(state: ref State): chan of int;       # Channel transport still requires the matching host adapter.
};                                                  # End the module interface; no runtime body is supplied.
