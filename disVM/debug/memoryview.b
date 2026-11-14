# Copyright (c) 2026 bhgv
# EN: Present a captured native pool partition without mixing separate sample instants.
# RU: Представить native pool partition, не смешивая разные моменты снятия counters.
implement Memoryview;
include "processctl.m";
include "memoryview.m";


# EN: Capture once through matched Processctl, construct caller-owned display data.
# No mutable globals, object walks or target pins; managed allocation errors propagate.
# RU: Снять один snapshot через matched Processctl, создать caller-owned display data.
# Без mutable globals, object walks либо target pins; managed allocation errors выходят.
inspect(): (int, ref Stats)
{
    ctl := load Processctl Processctl->PATH;
    (status, arena, header, live, free, blocks, holes, overhead, allocations, releases, peak, flags) := ctl->memorysnapshot();
    if (status != 1) {
        return (status, nil);
    }
    return (1, ref Stats(arena, header, live, free, blocks, holes, overhead, allocations,
                        releases, peak, flags, live - blocks*overhead, free - holes*overhead));
}
