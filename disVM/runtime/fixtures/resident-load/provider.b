# Copyright (c) 2026 bhgv
# EN: Keep independent module data live across a genuine native sleep.
# RU: Сохранить независимые данные модуля при настоящем native sleep.
implement Burst;
include "sys.m";
include "provider.m";
marker: int; # Private scalar in this loaded Modlink's MP.
roots: array of int; # Private mapped owner in this loaded Modlink's MP.


# EN: Return this instance's marker without allocating or touching peer instances.
# RU: Вернуть marker этого экземпляра без выделений и обращения к чужим экземплярам.
peek(): int
{
    return marker;
}


# EN: Store value in this instance, yield through Sys, validate and return it.
# RU: Сохранить value в экземпляре, уступить через Sys, проверить и вернуть его.
run(value: int): int
{
    sys := load Sys Sys->PATH;
    marker = value;
    roots = array[] of {value, value + 1};
    own := load Burst "$self";
    if (own == nil || own->peek() != value) {
        raise "self load changed module identity or MP";
    }
    own = nil;
    if (sys->sleep(0) != 0 || marker != value || roots[0] != value || roots[1] != value + 1) {
        raise "parallel load shared or lost private module data";
    }
    return marker * 3 + 7;
}
