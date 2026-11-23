# Copyright (c) 2026 bhgv
# EN: Load the graph bridge in Dis and test positional updates from AVM1.
# RU: Загрузить графовый мост в Dis и проверить позиционные обновления AVM1.
implement Client;
include "sys.m";
include "graph.m";

Client: module { init: fn(); };


# EN: Verify an existing child, a newly shared return identity and error rollback.
# RU: Проверить прежнего потомка, новую общую ссылку и сброс при отказе.
init()
{
    sys := load Sys Sys->PATH;
    graph := load Graphs "/rpc/mounted/flash-graph.dis";
    if (graph == nil) {
        raise sys->sprint("cannot load graph exports: %r");
    }
    Leaf, Box, TupleBox: import graph;
    original := ref Leaf(8);
    bytes := array[] of {byte 17, byte 0};
    box := ref Box(original, bytes);
    if (graph->nested(box) != original || original.count != 9 || box.inner != original ||
        box.untouched != bytes) {
        raise "nested graph update lost identity";
    }
    rejected := 0;
    {
        graph->fail(box);
    } exception {
        "*" => rejected = 1;
    }
    if (!rejected || original.count != 9) {
        raise "graph error committed an update";
    }
    returned := graph->replace(box, 37);
    if (returned == nil || returned == original || box.inner != returned || returned.count != 37 ||
        box.untouched != bytes || original.count != 9) {
        raise "result and positional update did not share an ID";
    }
    separate := graph->make(-2);
    if (separate == nil || separate == returned || separate.count != -2) {
        raise "ref-only result failed";
    }
    values := array[13] of ref Leaf;
    for (i := 0; i < len values; i++) {
        values[i] = ref Leaf(i);
    }
    values[11] = values[0];
    if (graph->fan(values) != values[0] || values[0] != values[11] ||
        values[0].count != 0 || values[12].count != 13) {
        raise "array of ref lost its sparse update or alias";
    }
    tuplebox := ref TupleBox((3, separate), bytes);
    child := graph->tupleedit(tuplebox, 52);
    (score, inpair) := tuplebox.pair;
    if (child != separate || inpair != separate || score != 52 || tuplebox.untouched != bytes) {
        raise "tuple field update lost an unchanged value";
    }
    sys->print("PASS DisVM loaded FlashVM graph exports\n");
}
