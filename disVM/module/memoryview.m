# Copyright (c) 2026 bhgv
# EN: Caller-owned display of one coherent native managed-arena accounting snapshot.
# RU: Caller-owned представление coherent native managed-arena accounting snapshot.
Memoryview: module
{
    PATH: con "memoryview.dis"; # Resolve beside the application image.


    # EN: Pool accounting only; capacity includes alignment and managed Heap headers.
    # Native metadata outside the arena, types/origins and OS RSS are not counted.
    # RU: Только pool accounting; capacity включает alignment и managed Heap headers.
    # Native metadata вне арены, types/origins и OS RSS не учитываются.
    Stats: adt {
        arena: big; # Whole calloc arena extent.
        header: big; # Arena header outside block partition.
        live: big; # Allocated spans, with native overhead/alignment.
        free: big; # Free spans; fragmentation may prevent a large allocation.
        liveblocks: big; # Native blocks, including unpublished construction buffers.
        freeblocks: big; # Free extents, not coalesced by this allocator.
        overhead: big; # Native per-block header/generation/tail bytes.
        allocations: big; # Successful allocations; bounded history.
        releases: big; # Successful frees; bounded history.
        peak: big; # Peak simultaneously allocated span.
        flags: int; # Processctl memory history saturation bits.
        livecapacity: big; # live minus liveblocks*overhead, not requested payload.
        freecapacity: big; # free minus freeblocks*overhead, not largest free extent.
    };


    # EN: Return status1/caller-owned Stats, or -1/nil on unavailable counters.
    # The returned ADT allocation happens after its captured accounting instant.
    # RU: Вернуть status1/caller-owned Stats либо -1/nil при недоступных counters.
    # Выделение возвращаемого ADT происходит после снятого accounting instant.
    inspect: fn(): (int, ref Stats);
};
