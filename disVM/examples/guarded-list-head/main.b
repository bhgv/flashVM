implement ListHead;

include "sys.m";

ListHead: module
{
    init: fn();
};

# The managed String makes each list head a mapped record. HEADMP must retain
# its fields, not merely copy pointer bytes out of a possibly retiring node.
Record: adt
{
    name: string;
    number: int;
};


# The same native-cell image works with default execution or guarded copies.
# A projected record owns its name independently of the remaining list nodes.
init()
{
    sys := load Sys Sys->PATH;
    items: list of Record;

    # Prepending reverses order: the first visible record is number 31.
    for (index := 0; index < 32; index++)
    {
        items = Record("node " + string index, index) :: items;
    }

    # Keep the copied head after removing the node from the list. Its String
    # must stay alive even if the old list node is queued for Fast retirement.
    expected := 31;
    while (items != nil)
    {
        head := hd items;
        items = tl items;
        if (head.number != expected || head.name != "node " + string expected)
        {
            raise "head payload changed after source-node release";
        }
        expected--;
    }
    if (expected != -1)
    {
        raise "list traversal missed a record";
    }
    sys->print("PASS guarded list head: 32 independent mapped records\n");
}
