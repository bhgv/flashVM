FIR 1.0 // Retain the existing textual format version.
module @Booleans // Name one standalone scalar initializer.
func @main() -> void { // Execute without parameters or cross-heap references.
^entry: // Define the first dominating basic block.
    %yes: bool = const.bool true // Materialize normalized one.
    %no: bool = const.bool false // Materialize normalized zero.
    %same: bool = bool.eq %yes, %no // Compare exact bool values to produce false.
    %different: bool = bool.not %same // Invert once to produce true.
    trace %different // Capture one true value: FlashVM currently prints 1.
    return // Finish the initializer without allocating an owner.
} // Link the emitted SWO through SWL to obtain an ordinary SWF8 file.
