implement Arithmetic;
include "arithmetic.m";

# A Dis implementation of the SAME interface used to generate the external AS side.
# Native bootstrap can compile this example today; full own Dis lowering is in progress.
add(a, b: int): int
{
    return a + b;
}

# Return a new string value; a future bridge copies/serializes it into the SWF owner.
greet(name: string): string
{
    return "Hello, " + name;
}
