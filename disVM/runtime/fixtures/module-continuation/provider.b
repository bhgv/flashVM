implement Provider;
include "sys.m";
include "provider.m";

marker: int;

touch(): int
{
    marker = 314159;
    return marker;
}

failheld(value: array of int)
{
    if (value[0] > 0)
        failheld(array[] of {value[0]-1});
    else
        raise "shared frame unwind";
}

heldvalue(value: array of int): int
{
    return value[0];
}

wait(): int
{
    sys := load Sys Sys->PATH;
    buffer := array[1] of byte;
    local := array[] of {271828};
    if (sys->read(sys->fildes(0), buffer, 1) != 1 ||
        marker != 314159 || local[0] != 271828)
        raise "provider state lost across native wait";
    if (buffer[0] == byte 'E')
        raise "provider exception after native wait";
    if (buffer[0] != byte 'R')
        raise "unexpected input";
    return marker+local[0];
}

# Exercise the cross-module spawn path with the provider's existing data.
spawned(done: chan of int)
{
    done <-= marker;
}
