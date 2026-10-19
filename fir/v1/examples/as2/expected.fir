FIR 1.0
module @tutorial
// Experimental as2 numeric tutorial profile.
func @main() -> void {
^entry:
    // Explicit temporaries expose evaluation order.
    // Временные значения явно показывают порядок вычисления.
    %v0: f64 = const.f64 20
    %v1: f64 = const.f64 6
    %v2: f64 = f64.sub %v0, %v1
    %v3: f64 = const.f64 3
    %v4: f64 = f64.mul %v2, %v3
    trace %v4
    return
}
