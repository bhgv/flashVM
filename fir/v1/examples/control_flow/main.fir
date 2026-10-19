FIR 1.0
// Hand-written FIR tutorial / Учебный FIR, написанный вручную.
module @control_flow
func @main() -> void {
^entry:
    // Two-way condition / Условие с двумя исходами.
    %a: i32 = const.i32 6
    %b: i32 = const.i32 7
    %less: bool = i32.lt.signed %a, %b
    if %less, ^yes, ^no
^yes:
    // Only this trace executes / Выполняется только этот trace.
    %message: string = const.string "yes / да"
    trace %message
    jump ^done
^no:
    %other: string = const.string "no / нет"
    trace %other
    jump ^done
^done:
    return
}
