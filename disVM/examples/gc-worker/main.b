implement GCWorker;

include "sys.m";
sys: Sys;

GCWorker: module
{
    init: fn();
};

# Each unreachable self-cycle needs the real cycle collector, not just RC.
# payload keeps an ordinary pointer-bearing array live through stack scans.
Node: adt
{
    next: ref Node;
    payload: array of int;
};


# Allocate a private cyclic graph; its references die when this call returns.
# seed initializes data so retained live roots can be checked independently.
# This function never changes a node owned by another Dis process.
cycle(seed: int)
{
    # This acyclic array exercises deferred last-owner RC independently of
    # the self-cycle below. Its contents must remain valid until scope exit.
    scratch := array[32] of int;
    scratch[0] = seed;
    node := ref Node(nil, array[64] of int);
    node.payload[0] = seed;
    node.next = node;
    if (scratch[0] != seed)
    {
        raise "fail: deferred RC lost local array";
    }
}


# Publish a cyclic node through a shared array slice. The command and reply
# channels release both writers and leave the shared cell as the only live root.
graphwriter(shared: array of ref Node, commands, replies: chan of int)
{
    for (;;)
    {
        seed := <-commands;
        if (seed < 0)
        {
            return;
        }
        {
            value := ref Node(nil, array[2] of int);
            value.next = value;
            value.payload[0] = seed;
            value.payload[1] = 314159-seed;
            shared[0:] = array[] of {value};
            raise "published graph";
        } exception failure {
            "published graph" => replies <-= seed;
            * => raise;
        }
    }
}


# Two guests replace the same mapped slot and unwind their local roots.
# A live slot must preserve its cycle while allocations trigger collection.
sharedgraph()
{
    shared := array[1] of ref Node;
    first := chan[1] of int;
    second := chan[1] of int;
    replies := chan[2] of int;
    spawn graphwriter(shared, first, replies);
    spawn graphwriter(shared, second, replies);
    for (round := 0; round < 32; round++)
    {
        low := 2*round;
        high := low+1;
        first <-= low;
        second <-= high;
        one := <-replies;
        two := <-replies;
        if (!((one == low && two == high) || (one == high && two == low)))
        {
            raise "shared graph writers did not both publish";
        }
        for (probe := 0; probe < 64; probe++)
        {
            cycle(probe);
        }
        if (shared[0] == nil || shared[0].next != shared[0])
        {
            raise "collector lost shared graph after writer unwind";
        }
        seed := shared[0].payload[0];
        if ((seed != low && seed != high) || shared[0].payload[1] != 314159-seed)
        {
            raise "shared graph contains a partial publication";
        }
        shared[0] = nil;
    }
    first <-= -1;
    second <-= -1;
    sys->print("PASS gc-worker shared graph: 64 unordered publications after unwind\n");
}


# A direct block above the deferred-byte budget must use ordinary destruction.
# Its first/last bytes are checked before scope exit; it must not occupy a queue
# slot until an arbitrary later collection. There is no shared mutable owner.
largeblock()
{
    data := array[6*1024*1024] of byte;
    data[0] = byte 17;
    data[len data-1] = byte 91;
    if (data[0] != byte 17 || data[len data-1] != byte 91)
    {
        raise "fail: large direct block data";
    }
}


# Keep a live cyclic root across many allocations and GC safepoints. Successful
# completion requires both heap reclamation and preservation of reachable data.
# No forced halt/process exit bypasses collector join or native TLS retirement.
init()
{
    sys = load Sys Sys->PATH;
    largeblock();
    survivor := ref Node(nil, array[64] of int);
    survivor.next = survivor;
    survivor.payload[0] = 314159;
    for (i := 0; i < 12000; i++)
    {
        cycle(i);
        if (survivor.next != survivor || survivor.payload[0] != 314159)
        {
            raise "fail: collector lost live cyclic root";
        }
    }
    sharedgraph();
    sys->print("PASS gc-worker: 12000 unreachable cycles, live root retained\n");
}
