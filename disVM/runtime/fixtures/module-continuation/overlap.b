implement Overlap;
include "sys.m";

Overlap: module { init: fn(); };

copy(id, factor: int, done: chan of int)
{
    product := id*factor;
    adjusted := product + factor;
    checksum := adjusted ^ factor;
    checksum += 19;
    checksum *= -3;
    if (checksum != -3*(((id*17 + 17) ^ 17) + 19))
        raise "parallel arithmetic lost its result";
    sum := 0;
    for (iteration := 0; iteration < 128; iteration++)
        sum += factor;
    if (sum != 2176)
        raise "parallel loop lost its result";
    narrow := byte 250;
    increment := byte factor;
    narrow += increment;
    narrow ^= byte 3;
    if (narrow != byte 8)
        raise "parallel BYTE wraparound changed";
    wide := big (id+1);
    wide <<= 36;
    delta := big factor;
    wide += delta;
    wide *= big 3;
    wide -= delta;
    if (wide != (((big (id+1)) << 36)*big 3 + big 34))
        raise "parallel LONG arithmetic lost high bits";
    source := array[] of {id, id+1, id+2};
    destination := array[3] of int;
    destination[0:] = source;
    if (destination[0] != id || destination[1] != id+1 || destination[2] != id+2)
        raise "parallel array copy lost its payload";
    done <-= id;
}

init()
{
    sys := load Sys Sys->PATH;
    done := chan of int;
    for (id := 0; id < 40; id++)
        spawn copy(id, 17, done);
    seen := array[40] of {* => 0};
    for (id = 0; id < 40; id++)
    {
        result := <-done;
        if (result < 0 || result >= len seen || seen[result])
            raise "duplicate parallel copy completion";
        seen[result] = 1;
    }
    sys->print("PASS forty dedicated shared copies\n");
}
