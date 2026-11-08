implement ContainerCycle;
include "sys.m";

ContainerCycle: module { init: fn(); };

Node: adt
{
    links: cyclic array of ref Node;
    chain: cyclic list of ref Node;
};

# The view owns its backing array. The array and list both lead back to node.
make(): ref Node
{
    storage := array[2] of ref Node;
    node := ref Node(storage[1:], nil);
    node.links[0] = node;
    node.chain = node :: node :: nil;
    return node;
}

init()
{
    sys := load Sys Sys->PATH;
    live := make();
    for (i := 0; i < 64; i++)
        make();
    sys->sleep(100);
    if (live.links[0] != live || hd live.chain != live || hd tl live.chain != live)
        raise "live container cycle changed";
    sys->print("PASS live group\n");
}
