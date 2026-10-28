FIR 1.0 // Every nonblank line explains an exact binary64 cell operation.
module @real_cells // Use a separate namespace for this independent object.
func @main() -> void { // No scalar cell handle escapes this entry.
^entry: // A constructor initializes the entire binary64 payload.
    %cell: f64.slot = f64.local // Start with positive zero and no GC root bit.
    %initial: f64 = f64.load %cell // Snapshot positive zero before mutation.
    %negative: f64 = const.f64 -0.0 // Keep the signed-zero payload explicit.
    f64.store %cell, %negative // Replace the scalar payload without retaining an owner.
    %saved: f64 = f64.load %cell // This immutable SSA value keeps negative zero.
    f64.store %cell, %initial // Mutating the cell again cannot change the earlier snapshot.
    trace %initial // Capture positive-zero bits as an ordinary numeric value.
    trace %saved // Capture negative-zero bits even after the later store.
    return // Both validity flags are one, and no heap resource remains live.
} // Human SWF trace formatting is separate from exact Dis bit inspection.
