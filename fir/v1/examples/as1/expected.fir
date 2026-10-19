FIR 1.0
module @tutorial
// Experimental as1 numeric tutorial profile.
func @main() -> void {
^entry:
    // Explicit temporaries expose evaluation order.
    // Временные значения явно показывают порядок вычисления.
    %v0: f64 = const.f64 2
    %v1: f64 = const.f64 3
    %v2: f64 = const.f64 4
    %v3: f64 = f64.mul %v1, %v2
    %v4: f64 = f64.add %v0, %v3
    trace %v4
    return
}
