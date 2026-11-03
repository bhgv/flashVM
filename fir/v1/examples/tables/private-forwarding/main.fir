FIR 1.0
module @private_forwarding
func @main() -> void {
^entry:
    %table: table.big = table.new
    %key: i64 = const.i64 0
    %value: i64 = const.i64 42
    %slot: table.slot = table.index %table, %key
    table.store %slot, %value
    table.lock %table
    %result: i64 = table.load %slot
    table.unlock %table
    trace %result
    return
}
