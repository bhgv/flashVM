FIR 1.0
module @tutorial
source_language as2
source_semantics avm1_numeric
func @main() -> void {
^entry:
    %v0: f64 = const.f64 123
    trace %v0
    return
}
