# Copyright (c) 2026 bhgv
#
# EN: Exercise Duke and Fast GC with a cycle sharing one large live array.
# RU: Проверка ДюкGC и Fast GC на цикле с общим большим живым массивом.

implement LiveBranchCharge;
include "sys.m";

LiveBranchCharge: module { init: fn(); };
Node: adt { next: cyclic ref Node; data: array of byte; };

# EN: Build a ring of length nodes, each referencing data.
# RU: Построить кольцо из length узлов, каждый хранит ссылку на data.
ring(length: int, data: array of byte): ref Node
{
    first := ref Node(nil, data);
    current := first;
    for (i := 1; i < length; i++)
    {
        current.next = ref Node(nil, data);
        current = current.next;
    }
    current.next = first;
    return first;
}

# EN: Retire an array of 300 references to the same live data.
# RU: Освободить массив из 300 ссылок на один живой объект data.
drop_aliases(data: array of byte)
{
    slots := array[300] of array of byte;
    for (i := 0; i < len slots; i++)
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
    ring(300, data);
    drop_aliases(data);
    sys->sleep(1200);
    if (int data[1] != 42)
        raise "live branch changed";
    sys->print("PASS live group\n");
}
