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


Publication: adt
{
    node: ref Node;
    seed: int;
};


# Shared module slots exercise MP ownership separately from frame slots.
module_node: ref Node;
module_record: Publication;


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
            # Publish two owning cells together and verify both after unwind.
            witness := ref Node(nil, array[1] of int);
            witness.next = value;
            witness.payload[0] = seed;
            shared[0:] = array[] of {value, witness};
            module_node = value;
            raise "published graph";
        } exception failure {
            "published graph" => replies <-= seed;
            * => raise;
        }
    }
}


# Two guests replace the same mapped range and unwind their local roots.
# The live range must preserve both nodes while allocations trigger collection.
sharedgraph()
{
    shared := array[2] of ref Node;
    first := chan[1] of int;
    second := chan[1] of int;
    replies := chan[2] of int;
    spawn graphwriter(shared, first, replies);
    spawn graphwriter(shared, second, replies);
    previous := Publication(nil, -1);
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
        if (shared[0] == nil || shared[1] == nil || shared[0].next != shared[0])
        {
            raise "collector lost shared graph after writer unwind";
        }
        seed := shared[0].payload[0];
        if ((seed != low && seed != high) || shared[0].payload[1] != 314159-seed ||
            shared[1].next != shared[0] || shared[1].payload[0] != seed)
        {
            raise "shared graph contains a partial publication";
        }
        # Force a mapped record copy with a live previous owner.
        current := Publication(shared[0], seed);
        module_record = current;
        previous = module_record;
        module_seed := module_node.payload[0];
        if (module_node.next != module_node || (module_seed != low && module_seed != high))
            raise "module data lost its published owner";
        module_node = nil;
        shared[0] = nil;
        shared[1] = nil;
        cycle(round);
        if (previous.node.next != previous.node || previous.seed != seed)
        {
            raise "mapped frame record lost its published reference";
        }
    }
    module_record = Publication(nil, -1);
    first <-= -1;
    second <-= -1;
    # Overlapping views share one backing array and preserve snapshot order.
    values := array[] of {ref Node(nil, nil), ref Node(nil, nil), ref Node(nil, nil)};
    left := values[0:2];
    right := values[1:3];
    firstvalue := values[0];
    secondvalue := values[1];
    # Drop the direct owner before entering the shared copy.
    values = nil;
    right[0:] = left;
    if (left[0] != firstvalue || left[1] != firstvalue || right[1] != secondvalue)
    {
        raise "overlapping view copy changed snapshot order";
    }
    # Exercise raw overlapping views with the same backing lifetime contract.
    raw := array[] of {10, 20, 30, 40};
    rawleft := raw[0:3];
    rawright := raw[1:4];
    raw = nil;
    rawright[0:] = rawleft;
    if (rawleft[0] != 10 || rawleft[1] != 10 ||
        rawright[1] != 20 || rawright[2] != 30)
        raise "overlapping raw view copy changed snapshot order";
    sys->print("PASS gc-worker shared graph: 64 unordered publications after unwind\n");
}


# Keep independent frame roots live while a deep callee publishes a mapped
# record. Exercise both normal return and exception unwinding across extents.
checkframes(depth, fail: int): int
{
    node := ref Node(nil, array[] of {depth});
    node.next = node;
    record := Publication(node, depth);
    result := 0;
    if (depth == 0)
    {
        copy := Publication(nil, -1);
        copy = record;
        cycle(depth);
        result = copy.node.payload[0] + copy.seed;
        if (fail != 0)
            raise "deep frame unwind";
    }
    else
    {
        {
            result = checkframes(depth-1, fail);
        } exception failure {
            "deep frame unwind" =>
                if (depth != 192)
                    raise;
            * => raise;
        }
    }
    if (record.node.next != node || record.node.payload[0] != depth ||
        record.seed != depth)
        raise "live caller frame lost its mapped root";
    return result+depth;
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
    if (checkframes(192, 0) != 18528 || checkframes(192, 1) != 192)
        raise "deep frame return or exception mismatch";
    # Give the event scheduler a clean interval after deferred owners drain.
    # The interval spans many turns even at the hosted test's larger quantum.
    total := 0;
    for (step := 0; step < 120000; step++)
        total += 1;
    if (total != 120000)
        raise "scalar interval changed";
    sys->print("PASS gc-worker deep frame return and unwind\n");
    sys->print("PASS gc-worker: 12000 unreachable cycles, live root retained\n");
}
