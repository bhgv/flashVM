implement ThreadedOwner;
include "sys.m";

# This complete managed application can run under disvm-editor --threaded-ui.
# Its Limbo spawn tasks remain cooperative within the one Dis OS owner. The UI
# main thread and its raster workers remain independently scheduled by the OS.
ThreadedOwner: module {
    init: fn(); # Run four real channel computations and return normally.
};


# Own a private managed scratch array; send copied scalar results at completion.
# lane/count are scalar inputs, reply is borrowed through its single rendezvous.
# This is a Dis process, not a claim that spawn itself creates an OS thread.
calculate(lane, count: int, reply: chan of (int,int,int))
{
    scratch := array[4096] of int;
    sum := 0;
    for (i := 1; i <= count; i++) {
        sum += i*lane;
        scratch[i%len scratch] = sum;
    }
    reply <-= (lane,sum,scratch[count%len scratch]);
}


# Load the actual Sys module, schedule four managed workers and validate results.
# Normal return requires the host to retire channels/processes and owner TLS;
# no /dev/sysctl exit bypass is used. Mismatches report FAIL and raise an error.
init()
{
    sys := load Sys Sys->PATH;
    if (sys == nil) {
        raise "fail: threaded-owner requires Sys";
    }
    replies := chan of (int,int,int);
    count := 10000;
    for (lane := 1; lane <= 4; lane++) {
        spawn calculate(lane,count,replies);
    }
    seen := array[5] of {* => 0};
    for (finished := 0; finished < 4; finished++) {
        (lane,total,last) := <-replies;
        if (lane < 1 || lane > 4 || seen[lane] || total != lane*count*(count+1)/2 || last != total) {
            sys->print("FAIL threaded-owner result\n");
            raise "fail: threaded-owner result";
        }
        seen[lane] = 1;
    }
    sys->print("PASS threaded-owner: four managed channel computations, normal owner retirement\n");
}
