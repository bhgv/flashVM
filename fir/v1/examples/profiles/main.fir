FIR 1.0
module @profiles
source_language as2
source_semantics avm1_numeric
func @main() -> void {
^entry:
    %answer: f64 = const.f64 +0x2a
    trace %answer
    %a: f64 = const.f64 +0x10
    %b: f64 = const.f64 +.5
    %quotient: f64 = f64.div %a, %b
    trace %quotient
    return
}
