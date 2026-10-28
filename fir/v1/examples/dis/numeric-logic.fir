FIR 1.0 // Private normalized bool cells can now lower through SWO and SWL.
module @numeric_logic_cells // Give this object a unique module-local namespace.
func @main() -> void { // No cell handle is returned or sent across a channel.
^entry: // Both cells are allocated before either control-flow successor.
    %first: bool.slot = bool.local // Initialize the first scalar frame cell to false.
    %second: bool.slot = bool.local // Allocate an independent cell, also false.
    %old: bool = bool.load %first // Snapshot the initial false value into immutable SSA.
    %yes: bool = const.bool true // A store accepts normalized bool values only.
    bool.store %first, %yes // Mutate only the first private cell.
    %now: bool = bool.load %first // Snapshot true without aliasing the mutable cell.
    %other: bool = bool.load %second // Observe that the second cell remains false.
    trace %old // Emit false, retained independently of the later store.
    trace %now // Emit true from the first cell after mutation.
    trace %other // Emit false from the untouched second cell.
    return // Retire both scalar cells with this frame; no GC owner is retained.
} // The SWF backend uses object-local variables and standard AVM1 actions.
