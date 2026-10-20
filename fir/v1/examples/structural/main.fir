FIR 1.0
// Structural syntax example; use fir1 syntax, not object generation for this profile.
module @structural
source_language as2
source_semantics flash8
file !1 "sum.as"
loc !10 !1:1:1
export @sum

// Function/block parameters are explicit SSA values, not mutable variables.
func @sum(%count: i32) -> i32 {
^entry:
    %zero: i32 = const.i32 0
    jump ^loop(%zero, %zero)

^loop(%index: i32, %total: i32):
    %more: bool = i32.lt.signed %index, %count loc !10
    if %more, ^body, ^done(%total)

^body:
    %next_total: i32 = i32.add.wrap %total, %index
    %one: i32 = const.i32 1
    %next_index: i32 = i32.add.wrap %index, %one
    jump ^loop(%next_index, %next_total)

^done(%answer: i32):
    return %answer
}
