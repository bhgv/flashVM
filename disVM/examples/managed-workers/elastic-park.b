implement ManagedElasticPark;
include "sys.m";
sys: Sys; # Used for a visible setup marker, not for a simulated native wait.

ManagedElasticPark: module {
    init: fn(); # Entry returns while independent logical listeners remain alive.
};


# The task remains rooted and parked until a future channel producer appears.
# It owns no permanently assigned OS thread; the test host ends this process.
listen(input: chan of int)
{
    value := <-input;
    if (value != 1) {
        raise "fail: parked listener value";
    }
}


# Returning from module entry must not silently discard256 blocked tasks in
# automatic mode. The driver observes the quiescent state before host cleanup.
init()
{
    sys = load Sys Sys->PATH;
    input := chan of int;
    for (i := 0; i < 256; i++) {
        spawn listen(input);
    }
    sys->print("ELASTIC_PARKED\n");
}
