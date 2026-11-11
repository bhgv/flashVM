implement ManagedRC;
include "sys.m";
sys: Sys; # Immutable interface; parent native wait pins its original continuation.

ManagedRC: module {
    init: fn(); # External driver withholds stdin until helpers finish allocating/releasing.
};

Node: adt {
    value: int; # Private scalar payload checked before its containing array is released.
};


# Each actor repeatedly replaces an array of sixteen owned nodes. The previous
# array's last reference can transfer to Fast GC; nested collector destruction
# must not enqueue itself. done carries the independent scalar checksum only.
produce(done: chan of int)
{
    values: array of ref Node;
    total := 0;
    for (round := 0; round < 16; round++) {
        values = array[16] of ref Node;
        for (index := 0; index < len values; index++) {
            values[index] = ref Node(round + index);
            total += values[index].value;
        }
        # Native suspension lets peer threads progress while this thread waits.
        # Each process retains its OS owner and live registers until termination.
        sys->sleep(1);
    }
    values = nil;
    done <-= total;
}


# Aggregate every producer while the parent Dis process is held in a real native read.
# Its output is the driver's permission to release stdin, never a timing guess.
collect(done, finished: chan of int)
{
    total := 0;
    for (index := 0; index < 32; index++) {
        value := <-done;
        if (value != 3840) {
            raise "fail: helper RC payload changed";
        }
        total += value;
    }
    if (total != 122880) {
        raise "fail: helper RC checksum";
    }
    sys->print("PASS helper RC actors before stdin\n");
    finished <-= total;
}


# Start helper work and then remain outside managed execution until the driver
# observes completion. Native sleeps distribute producer continuations across
# OS owners; controller-only RC admission cannot satisfy helper_enqueued. Native read roots stay valid during Fast/Cycles GC.
init()
{
    sys = load Sys Sys->PATH;
    done := chan of int;
    finished := chan of int;
    spawn collect(done, finished);
    for (index := 0; index < 32; index++) {
        spawn produce(done);
    }
    input := sys->fildes(0);
    token := array[1] of byte;
    sys->print("HELPER_RC_READY\n");
    if (sys->read(input, token, 1) != 1 || token[0] != byte '1') {
        raise "fail: helper RC handshake";
    }
    if (<-finished != 122880) {
        raise "fail: helper RC completion";
    }
    sys->print("PASS helper RC completed\n");
}
