implement MillisecTest;
include "sys.m";

MillisecTest: module {
    init: fn(); # Self-contained host-service check, no GUI or Flash module.
};


# Call the real native Sys entry repeatedly. No sleeps, channels, imports beyond
# Sys or frame-timed callbacks are required. Host test rejects any FAIL output.
init()
{
    sys := load Sys Sys->PATH;
    previous := sys->millisec();
    for (index := 0; index < 2000; index++) {
        now := sys->millisec();
        if (now-previous < 0) {
            sys->print("FAIL millisec: clock moved backwards\n");
            return;
        }
        previous = now;
    }
    sys->print("PASS Sys.millisec: 2001 native calls without GUI or Flash frames\n");
}
