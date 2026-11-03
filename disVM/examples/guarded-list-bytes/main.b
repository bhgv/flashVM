implement ScalarHeads;

include "sys.m";

ScalarHeads: module
{
    init: fn();
};

# No managed references: extracting this record copies bytes, not owners.
Pair: adt
{
    left: int;
    right: int;
};


# Exercise all five scalar/raw head widths using one compiled native-cell image.
# Takes no parameters, prints the checked values and raises on a wrong result.
# Source lists remain owned throughout; asynchronous GC uses its normal safepoints.
init()
{
    sys := load Sys Sys->PATH;
    bytes := byte 137 :: nil;
    words := 731 :: nil;
    longs := big 9000000007 :: nil;
    reals := 3.5 :: nil;
    pairs := Pair(3, 7) :: nil;

    # These expressions select HEADB, HEADW, HEADL, HEADF and HEADM respectively.
    b := hd bytes;
    w := hd words;
    l := hd longs;
    f := hd reals;
    p := hd pairs;
    if (int b != 137 || w != 731 || l != big 9000000007 ||
        f != 3.5 || p.left != 3 || p.right != 7)
    {
        raise "scalar list projection changed bits";
    }
    sys->print("PASS guarded scalar lists: 137 731 9000000007 3.5 3 7\n");
}
