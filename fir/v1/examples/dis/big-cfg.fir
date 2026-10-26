FIR 1.0
// Direct FIR example: full-width big arithmetic across explicit control-flow blocks.
// This is executable FIR input for our Coco/R parser, not a complete Limbo module.
module @BigDecision
func @main() -> void {
^entry:
    // These existing i32 comparisons supply a bool decision. Big comparisons are
    // not introduced by this example; no implicit conversion mixes the two types.
    %zero: i32 = const.i32 0
    %one: i32 = const.i32 1
    %choose: bool = i32.lt.signed %zero, %one
    // Values defined in entry dominate both arms and survive scheduler slices.
    %a: i64 = const.i64 -9007199254740993
    %b: i64 = const.i64 97
    if %choose, ^quotient, ^complement
^done:
    // Placing the exit before both arms exercises backward instruction-PC fixups.
    // No phi is necessary: trace captures are separate module-data cells.
    return
^quotient:
    // Division truncates toward zero; the signed remainder reconstructs a exactly.
    %q: i64 = big.div %a, %b
    %r: i64 = big.mod %a, %b
    %product: i64 = big.mul %q, %b
    %restored: i64 = big.add %product, %r
    // The first trace value is the exact original a, beyond binary64 integer precision.
    trace %restored
    jump ^done
^complement:
    // This arm is verified and emitted, but not executed with the decision above.
    // Complement touches all 64 bits and uses an ordinary, unmapped scalar mask.
    %flipped: i64 = big.not %a
    trace %flipped
    jump ^done
}
