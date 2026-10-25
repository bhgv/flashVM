FIR 1.0
// Equivalent to main.b: all arithmetic wraps at each signed i32 operation.
module @DisAnswer
source_language limbo
source_semantics limbo_i32
func @main() -> void {
^entry:
    %maximum: i32 = const.i32 2147483647
    %one: i32 = const.i32 1
    %minus: i32 = const.i32 -1
    %wrapped: i32 = i32.add.wrap %maximum, %one
    %product: i32 = i32.mul.wrap %wrapped, %minus
    %answer: i32 = i32.sub.wrap %product, %one
    // SWO traces this value; the NC64 harness captures it at module-data byte zero.
    trace %answer
    return
}
