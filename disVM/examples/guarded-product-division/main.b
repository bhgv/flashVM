implement ProductDivision;

include "sys.m";

ProductDivision: module
{
    init: fn();
};


# Demonstrate nine native-cell integer product/quotient/remainder operations.
# No arguments or returned owners; raise on an incorrect value, print on success.
# The guarded runtime keeps both input cells and result excluded until publication.
init()
{
    sys := load Sys Sys->PATH;

    # Byte products wrap to eight bits: 200*3=600, whose low byte is 88.
    # Byte division/remainder use unsigned values; the divisor stays nonzero.
    small := byte 200;
    smallDivisor := byte 3;
    smallProduct := small * smallDivisor;
    smallQuotient := small / smallDivisor;
    smallRemainder := small % smallDivisor;
    if (int smallProduct != 88 || int smallQuotient != 66 || int smallRemainder != 2)
    {
        raise "byte product/division changed";
    }

    # Signed quotient truncates toward zero. Remainder has the dividend's sign.
    # Native WORD width belongs to this image profile, not a Dis64 wire rule.
    word := -73;
    wordDivisor := 5;
    wordProduct := word * wordDivisor;
    wordQuotient := word / wordDivisor;
    wordRemainder := word % wordDivisor;
    if (wordProduct != -365 || wordQuotient != -14 || wordRemainder != -3)
    {
        raise "word product/division changed";
    }

    # Big integers use the same capture-before-publication transaction. These
    # inputs avoid overflow; the native diagnostic suite covers MIN/-1 separately.
    wide := big -9000000007;
    wideDivisor := big 3;
    wideProduct := wide * wideDivisor;
    wideQuotient := wide / wideDivisor;
    wideRemainder := wide % wideDivisor;
    if (wideProduct != big -27000000021 || wideQuotient != big -3000000002 || wideRemainder != big -1)
    {
        raise "big product/division changed";
    }
    sys->print("PASS guarded products/division: nine typed operations\n");
}
