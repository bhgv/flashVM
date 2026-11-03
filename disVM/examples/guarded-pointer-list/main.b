implement PointerList;

include "sys.m";

PointerList: module
{
    init: fn();
};


# String list elements are managed pointers. HEADP copies a new owner of one
# element; TAIL copies a new owner of the next node before retiring old output.
# In particular, assigning tl items back to items aliases source and destination.
init()
{
    sys := load Sys Sys->PATH;
    items := "front" :: "middle" :: "back" :: nil;

    # The projected String must survive removal of the node that contained it.
    first := hd items;
    items = tl items;
    if (first != "front" || hd items != "middle")
    {
        raise "pointer head or aliased tail lost ownership";
    }

    # Repeated tail assignment removes the final nodes. The copied head remains
    # a separate owner, while the terminal tail is a valid nil list value.
    last := first;
    count := 0;
    while (items != nil)
    {
        last = hd items;
        items = tl items;
        count++;
    }
    if (count != 2 || last != "back" || first != "front")
    {
        raise "list traversal changed retained String values";
    }
    sys->print("PASS guarded pointer list: HEADP, aliased TAIL, terminal nil\n");
}
