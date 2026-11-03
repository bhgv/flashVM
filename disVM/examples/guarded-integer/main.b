implement IntegerConversions;

include "sys.m";

IntegerConversions: module
{
    init: fn();
};


# Demonstrate byte/word/long conversions with the same native-cell image.
# No arguments or returned owners. Raise on an incorrect value, otherwise print.
# The runtime guards input and output together when --guarded-copies is selected.
init()
{
    sys := load Sys Sys->PATH;
    for (index := 0; index < 256; index++)
    {
        # Narrow to a byte, widen without sign extension, then round-trip via big.
        # Each conversion captures its input before publishing the result.
        small := byte index;
        word := int small;
        wide := big word;
        back := int wide;
        if (word != index || back != index)
        {
            raise "integer conversion changed the value";
        }
    }
    sys->print("PASS guarded integer: all 256 byte values\n");
}
