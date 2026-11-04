implement RealArithmetic;
include "sys.m";

RealArithmetic: module
{
    init: fn();
};


# Run four existing REAL operations on dynamically read array elements.
# No arguments or borrowed resources. Sys is retained for the module's lifetime.
# A failed load or unexpected result raises an ordinary Limbo exception.
init()
{
    sys := load Sys Sys->PATH;
    if (sys == nil)
    {
        raise "load Sys";
    }
    # An array prevents the expressions below from becoming constant-folded
    # literals. The array is allocated before the four numerical expressions.
    values := array[] of {2.0, 4.0};
    left := values[0];
    right := values[1];
    # With --gc-worker --guarded-copies, each expression snapshots its inputs,
    # computes without shared guards, then publishes its result separately.
    # This does not promise an atomic update to a concurrently shared variable.
    sum := left + right;
    difference := left - right;
    product := left * right;
    quotient := left / right;
    if (sum != 6.0 || difference != -2.0 || product != 8.0 || quotient != 0.5)
    {
        raise "unexpected REAL result";
    }
    sys->print("REAL: sum=%g difference=%g product=%g quotient=%g\n",
        sum, difference, product, quotient);
}
