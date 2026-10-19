FIR 1.0
module @tutorial
// Experimental as3 numeric tutorial profile.
func @main() -> void {
^entry:
    %v0: f64 = const.f64 6
    %v1: f64 = const.f64 7
    %v2: f64 = f64.mul %v0, %v1
    %v3: f64 = f64.sub %v2, %v0
    trace %v3
    trace %v2
    return
}
