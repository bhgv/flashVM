# Produce the existing scalar Dis64 pilot with the temporary compiler's -6 flag.
# This is a standalone scalar entry, not the ordinary Sys->Context shell ABI.
implement ScalarLoop;

ScalarLoop: module {
    init: fn(); # The only exported entry; no arguments or managed return value.
};

counter: int; # WORD32 module storage, initialized to zero by the image loader.
answer: big; # Eight-byte signed scalar; this is not a heap reference.


# Execute a self-checking scalar loop. No parameters/result, allocations, imports,
# channels or external I/O are used. A wrong result deliberately divides by zero
# so a complete experimental emulator must report an error, never false success.
init()
{
    preserved := big 17; # Keep a Big local live across WORD scratch operations.
    counter = 0;
    answer = big 0;
    for (index := 0; index < 4097; index++) {
        counter++;
        answer += big 3;
    }
    if (counter != 4097 || answer != big 12291 || preserved != big 17) {
        counter /= 0;
    }
}
