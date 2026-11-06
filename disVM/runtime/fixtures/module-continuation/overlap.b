implement Overlap;
include "sys.m";
include "provider.m";

Overlap: module { init: fn(); };

published: int;
sample: real;
sharedarray: array of int;

scalarcall(value: int, held: array of int): int
{
    if (value == 0)
        return held[0];
    return 1 + scalarcall(value - 1, held);
}

copy(id, factor: int, done, resume: chan of int, provider: Provider)
{
    if (scalarcall(id % 4, array[] of {1}) != id % 4 + 1)
        raise "parallel scalar frame lost its return value";
    if (provider->heldvalue(array[] of {id}) != id)
        raise "parallel module call lost its return value";
    {
        provider->failheld(array[] of {id});
        raise "provider did not raise";
    } exception failure {
        "shared frame unwind" =>
            ;
        * =>
            raise failure;
    }
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
    divisor := factor - 14;
    quotient := checksum / divisor;
    remainder := checksum % divisor;
    if (quotient*divisor + remainder != checksum || remainder != 0)
        raise "parallel WORD division changed";
    wide /= big divisor;
    wide %= big factor;
    if (wide != ((((big (id+1)) << 36)*big 3 + big 34) / big 3) % big 17)
        raise "parallel LONG division changed";
    narrow /= byte divisor;
    narrow %= byte divisor;
    if (narrow != byte 2)
        raise "parallel BYTE division changed";
    narrow <<= divisor;
    narrow >>= divisor - 1;
    checksum <<= divisor;
    checksum >>= divisor;
    wide <<= divisor;
    wide >>= divisor;
    if (narrow != byte 4 || checksum != quotient*divisor || wide !=
        ((((big (id+1)) << 36)*big 3 + big 34) / big 3) % big 17)
        raise "parallel shift changed its result";
    zero := divisor - 3;
    preserved := id;
    {
        preserved += factor;
        preserved /= zero;
        raise "parallel division accepted a zero divisor";
    } exception failure {
        "zero divide" =>
            if (preserved != id + factor)
                raise "parallel division lost the preceding result";
        * =>
            raise failure;
    }
    source := array[] of {id, id+1, id+2};
    destination := array[3] of int;
    destination[0:] = source;
    if (destination[0] != id || destination[1] != id+1 || destination[2] != id+2)
        raise "parallel array copy lost its payload";
    same := source;
    same = source;
    if (same != source)
        raise "parallel reference copy changed an equal destination";
    same = nil;
    sharedarray = source;
    retained := sharedarray;
    source = nil;
    destination = nil;
    if (retained == nil || len retained != 3 || retained[1] != retained[0]+1)
        raise "parallel module reference lost its referent";
    published += factor;
    sample = 1.25;
    value := sample;
    if (value != 1.25)
        raise "parallel REAL copy changed its value";
    done <-= id;
    <-resume;
    if (len retained != 3 || retained[2] != retained[0]+2)
        raise "retained module reference failed after the slot was cleared";
    done <-= id;
}

init()
{
    sys := load Sys Sys->PATH;
    provider := load Provider "./provider.dis";
    done := chan of int;
    resume := chan of int;
    for (id := 0; id < 40; id++)
        spawn copy(id, 17, done, resume, provider);
    seen := array[40] of {* => 0};
    for (id = 0; id < 40; id++)
    {
        result := <-done;
        if (result < 0 || result >= len seen || seen[result])
            raise "duplicate parallel copy completion";
        seen[result] = 1;
    }
    if (published != 680)
        raise "parallel module increments lost updates";
    sharedarray = nil;
    for (id = 0; id < 40; id++)
    {
        resume <-= 1;
        result := <-done;
        if (result < 0 || result >= len seen || seen[result] != 1)
            raise "duplicate retained reference completion";
        seen[result] = 2;
    }
    sys->print("PASS forty dedicated shared copies\n");
}
