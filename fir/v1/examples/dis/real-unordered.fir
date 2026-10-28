FIR 1.0 // Retain the existing textual format and annotate its physical header.
module @RealUnordered // Name one standalone scalar initializer.
func @main() -> void { // Enter without parameters or external ownership.
^entry: // Dominate every operand and all three captured results.
    %zero: f64 = const.f64 0.0 // Only finite constants are admitted in FIR text.
    %nan: f64 = f64.div %zero, %zero // Produce an unordered value during execution.
    %equal: bool = f64.eq %nan, %nan // IEEE equality rejects even the same NaN value.
    %less: bool = f64.lt %nan, %zero // Ordered less-than is false for an unordered operand.
    %le: bool = f64.le %zero, %nan // Ordered <= is false; it is not inverted greater-than.
    trace %equal // Capture false in scalar Dis and the linked SWF.
    trace %less // Capture false through the same ordinary compiler/linker pipeline.
    trace %le // Capture false without changing the player's dynamic equality policy.
    return // Finish with no table, string or remote owner to release.
} // SWL wraps the verified SWO actions into an ordinary SWF8 container.
