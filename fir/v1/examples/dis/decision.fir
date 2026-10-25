FIR 1.0
// Equivalent to decision.b; physical order determines capture slots, not execution order.
module @Decision
source_language limbo
source_semantics limbo_i32
func @main() -> void {
^entry:
    %maximum: i32 = const.i32 2147483647
    %one: i32 = const.i32 1
    %wrapped: i32 = i32.add.wrap %maximum, %one
    %zero: i32 = const.i32 0
    %negative: bool = i32.lt.signed %wrapped, %zero
    // Source >= reverses the two edges of signed less-than; no bool-to-int cast is needed.
    if %negative, ^negative, ^nonnegative
^nonnegative:
    %unexpected: i32 = const.i32 -1
    trace %unexpected
    return
^negative:
    %minimum: i32 = const.i32 -2147483648
    %exact: bool = i32.eq %wrapped, %minimum
    if %exact, ^answer, ^other
^answer:
    %answer: i32 = const.i32 42
    trace %answer
    return
^other:
    // Value zero and execution flag zero are separate observations.
    trace %zero
    return
}
