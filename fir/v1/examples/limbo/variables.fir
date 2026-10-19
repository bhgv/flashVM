FIR 1.0
module @tutorial
// Experimental limbo numeric tutorial profile.
func @main() -> void {
^entry:
    %v0: i32 = const.i32 6
    %v1: i32 = const.i32 7
    %v2: i32 = i32.mul.wrap %v0, %v1
    %v3: i32 = i32.sub.wrap %v2, %v0
    trace %v3
    return
}
