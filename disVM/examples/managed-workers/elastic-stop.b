implement ManagedElasticStop;
include "sys.m";
sys: Sys; # Shared immutable interface; each blocked read retains its own FD root.

ManagedElasticStop: module {
    init: fn(); # Controlled fatal error while four native reads are still held.
};


# Announce before entering the native read. Driver supplies one byte per reader
# ONLY after observing all four leases and the parent's managed error report.
read(ready: chan of int)
{
    input := sys->fildes(0);
    token := array[1] of byte;
    ready <-= 1;
    if (sys->read(input, token, 1) != 1) {
        raise "fail: elastic stop reader";
    }
}


# Keep many helpers in the registry while four actors wait natively. Queue
# fairness gives each ready reader a turn during the scalar loop; the driver
# independently requires the actual native lease marker, not elapsed time.
init()
{
    sys = load Sys Sys->PATH;
    ready := chan of int;
    for (i := 0; i < 4; i++) {
        spawn read(ready);
    }
    for (i = 0; i < 4; i++) {
        if (<-ready != 1) {
            raise "fail: elastic stop readiness";
        }
    }
    value := 0;
    for (i = 0; i < 2000; i++) {
        value += i;
    }
    if (value != 1999000) {
        raise "fail: elastic stop computation";
    }
    raise "fail: elastic controlled stop";
}
