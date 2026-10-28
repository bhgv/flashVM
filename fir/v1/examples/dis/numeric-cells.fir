FIR 1.0 // Use existing textual FIR with append-only private scalar cell operations.
module @numeric_cells // Give generated SWF variables an object-local namespace.
func @main() -> void { // This entry owns all scalar cells until return.
^entry: // Constructors dominate every load, store and loop edge below.
    %sum: i32.slot = i32.local // Initialize the sum cell to exact signed32 zero.
    %index: i32.slot = i32.local // Allocate independent loop state, also zero.
    %one: i32 = const.i32 1 // Keep immutable arithmetic inputs outside the loop.
    %seven: i32 = const.i32 7 // Add seven on each of the six iterations.
    %six: i32 = const.i32 6 // The loop condition uses the current index snapshot.
    jump ^test // Enter the test before the first body execution.
^test: // Each backedge must execute fresh loads rather than stale SSA hints.
    %i: i32 = i32.load %index // Snapshot the current loop index.
    %more: bool = i32.lt.signed %i, %six // Keep the condition normalized and strictly typed.
    if %more, ^body, ^exit // A zero-iteration exit would still see initialized cells.
^body: // Memory effects prevent scalar updates from being hoisted across edges.
    %old: i32 = i32.load %sum // Snapshot the current accumulator independently.
    %next: i32 = i32.add.wrap %old, %seven // Preserve the existing modulo-32 arithmetic contract.
    i32.store %sum, %next // Publish the update only into the accumulator cell.
    %step: i32 = i32.add.wrap %i, %one // Compute the next index from this iteration's snapshot.
    i32.store %index, %step // Commit the index before returning to the test.
    jump ^test // Both cells now carry the next iteration's state.
^exit: // The final load observes all six completed accumulator writes.
    %result: i32 = i32.load %sum // Snapshot the exact result 42.
    trace %result // Capture one int payload and one validity flag in Dis.
    return // Retire both nonescaping scalar cells with this frame.
} // SWO/SWL uses ordinary AVM1 variables and backward branch fixups.
