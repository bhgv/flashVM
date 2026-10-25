# Dis-owned editing policy. AS views receive snapshots and geometry, never this mutable state.
Ideinput: module
{
    PATH: con "ideinput.dis";  # Relative resource resolved by the embedding host.


    # One controller process owns these fields; never share a mutable State between workers.
    State: adt
    {
        caret: int;     # Unicode character offset, clamped against the current document.
        anchor: int;    # Other selection endpoint; equal to caret for a collapsed selection.
        control: int;   # Published Control-key state, cleared on focus loss.
        shift: int;     # Published Shift-key state, cleared on focus loss.
        preferred: int; # Desired logical column across repeated vertical moves; -1 resets it.
        page: int;      # Logical lines per page, bounded 1..128 by the controller.
        revision: int;  # Last observed document revision, or -1 before initialization.
    };


    # Immutable-by-contract presentation value inside Dis; adapters copy it before crossing heaps.
    View: adt
    {
        document: Idecore->Reply;      # Complete existing service reply/snapshot.
        caret: int;                    # Current cursor offset in Unicode characters.
        anchor: int;                   # Current selection anchor in Unicode characters.
        handled: int;                  # One for implemented input, zero for unhandled/stale input.
        event: ref Uibridge->Message;  # Original value for higher-level routing or diagnostics.
    };


    # Return a fresh controller state at offset zero, with released modifiers and unknown revision.
    create: fn(): ref State;


    # core/document are borrowed; state is exclusively owned; event is borrowed and read-only.
    # Uses real document requests/replies. Returns (1, view), or (0, nil) for invalid arguments.
    # Text inserts; arrows/PageUp/PageDown/Home/End select; Backspace/Delete edit; Ctrl+A/Z/Y act in Dis.
    # Pointer/layout and other keys return handled=0 for composition with another Dis service.
    apply: fn(core: Idecore, document: chan of Idecore->Request,
              state: ref State, event: ref Uibridge->Message): (int, ref View);


    # Apply an acknowledged bounded selection without editing text/history.
    # state is exclusive, reply is a fresh owner snapshot; stale/closed/range errors return zero.
    # Returns one after setting anchor=first/caret=last and resetting the vertical goal.
    select: fn(state: ref State, reply: Idecore->Reply, first, last: int): int;


    # Incorporate an externally acknowledged document command without reinterpreting it in AS.
    # state is exclusive; reply is copied. Changed revisions reset selection deterministically.
    # Returns nothing; the bridge publishes the resulting presentation values separately.
    observe: fn(state: ref State, reply: Idecore->Reply);


    # Sequential controller owner. Borrowed input/views must be drained by the application.
    # A nil input sentinel stops the controller without shutting down the borrowed document owner.
    # Returns zero on sentinel shutdown, minus one for invalid inputs; no OS/UI module is imported.
    serve: fn(core: Idecore, document: chan of Idecore->Request,
              input: chan of ref Uibridge->Message, views: chan of ref View): int;
};
