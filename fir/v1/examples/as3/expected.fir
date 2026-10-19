FIR 1.0
module @tutorial
// Experimental as3 numeric tutorial profile.
func @main() -> void {
^entry:
    // Explicit temporaries expose evaluation order.
    // Временные значения явно показывают порядок вычисления.
    %v0: f64 = const.f64 2.5
    %v1: f64 = const.f64 3.25
    %v2: f64 = f64.add %v0, %v1
    trace %v2
    return
}
