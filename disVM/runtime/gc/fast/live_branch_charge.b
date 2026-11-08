# Copyright (c) 2026 bhgv
#
# EN: Exercise Duke and Fast GC with a cycle sharing one large live array.
# RU: Проверка ДюкGC и Fast GC на цикле с общим большим живым массивом.

implement LiveBranchCharge;
include "sys.m";

LiveBranchCharge: module { init: fn(); };
Node: adt
{
    next: cyclic ref Node;
    data: array of byte;
    branches: array of array of byte;
};

# EN: Build a ring of length nodes, each referencing data and branches.
# RU: Построить кольцо из length узлов, каждый хранит ссылки на data и branches.
ring(length: int, data: array of byte, branches: array of array of byte): ref Node
{
    first := ref Node(nil, data, branches);
    current := first;
    for (i := 1; i < length; i++)
    {
        current.next = ref Node(nil, data, branches);
        current = current.next;
    }
    current.next = first;
    return first;
}

# EN: Retire a sparse array with 300 references to the same live data.
# RU: Освободить разреженный массив с 300 ссылками на один живой объект data.
drop_aliases(data: array of byte)
{
    slots := array[70000] of array of byte;
    for (i := 0; i < 300; i++)
    {
        slots[i] = data;
    }
}

# EN: Drop the ring and verify the array still holds its value.
# RU: Отпустить кольцо и проверить, что массив сохранил значение.
init()
{
    sys := load Sys Sys->PATH;
    data := array[32768] of byte;
    data[1] = byte 42;
    branches := array[8] of array of byte;
    branches[0] = data;
    ring(300, data, branches);
    drop_aliases(data);
    sys->sleep(1200);
    if (int data[1] != 42 || branches[0] != data)
    {
        raise "live branch changed";
    }
    sys->print("PASS live group\n");
}
