FIR 1.0
module @tutorial
// Experimental limbo numeric tutorial profile.
func @main() -> void {
^entry:
    // Explicit temporaries expose evaluation order.
    // Временные значения явно показывают порядок вычисления.
    %v0: i32 = const.i32 2147483647
    %v1: i32 = const.i32 1
    %v2: i32 = i32.add.wrap %v0, %v1
    trace %v2 // Tutorial harness prints value().
    return
}
