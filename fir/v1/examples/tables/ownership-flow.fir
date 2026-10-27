FIR 1.0
// Explicit retained ownership across aliases and a control-flow join.
// This is executable FIR 1.0, independent of the Limbo source parser.
module @ownership_flow
func @main() -> void {
^entry:
    // Construct private storage; no other process can observe initialization.
    %table: table.big = table.new
    %zero: i64 = const.i64 0
    %initial: i64 = const.i64 40
    %two: i64 = const.i64 2
    %slot: table.slot = table.index %table, %zero
    table.store %slot, %initial

    // Copies preserve owner identity. Two entries require two balanced leaves.
    table.lock %table
    %alias: table.big = copy %table
    table.lock %alias
    %old: i64 = table.load %slot
    %answer: i64 = big.add %old, %two
    table.store %slot, %answer
    table.unlock %alias

    // Both incoming paths to join must retain exactly one level of this owner.
    %choose: bool = const.bool true
    if %choose, ^left, ^right
^left:
    jump ^join
^right:
    // A temporary recursive level is balanced within this arm.
    table.lock %alias
    table.unlock %table
    jump ^join
^join:
    table.unlock %table
    %result: i64 = table.load %slot
    trace %result
    // A normal FIR return requires zero retained levels for every owner.
    return
}
