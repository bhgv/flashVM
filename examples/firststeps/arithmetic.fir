FIR 1.0
module @tutorial
source_language as2
source_semantics avm1_numeric
func @main() -> void {
^entry:
    %v0: f64 = const.f64 8
    %v1: f64 = const.f64 2
    %v2: f64 = f64.sub %v0, %v1
    %v3: f64 = const.f64 7
    %v4: f64 = f64.mul %v2, %v3
    trace %v4
    return
}
