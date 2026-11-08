implement LargeCycleProbe;
include "sys.m";
LargeCycleProbe: module { init: fn(); };
Node: adt { next: ref Node; };
# Build a closed chain of length nodes and return its first node.
ring(length: int): ref Node
{
    first := ref Node(nil);
    current := first;
    for(i := 1; i < length; i++)
    {
        current.next = ref Node(nil);
        current = current.next;
    }
    current.next = first;
    return first;
}
# Drop the large ring and retain a separate ring to check that live links survive collection.
init()
{
    sys := load Sys Sys->PATH;
    live := ring(40);
    ring(400);
    sys->sleep(250);
    current := live;
    for(i := 0; i < 40; i++)
        current = current.next;
    if(current != live)
        raise "live ring changed";
    sys->print("PASS live group\n");
}
