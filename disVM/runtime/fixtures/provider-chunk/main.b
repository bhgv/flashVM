# Copyright (c) 2026 bhgv
# EN: Actual Sys acknowledgements from partial/error/zero-progress native providers.
# RU: Настоящие Sys confirmations при partial/error/zero-progress native providers.
implement ChunkCheck;
include "sys.m";
ChunkCheck: module { init: fn(); };
sys: Sys; # Immutable ordinary Sys builtin after initialization.


# EN/RU: Raise label when an observable guest contract fails.
# Вызвать raise label при нарушении наблюдаемого guest contract.
check(ok: int, label: string)
{
    if (!ok) {
        raise label;
    }
}


# EN/RU: Require precise read/write prefixes, stop readn on partial refusal and
# expose no-progress write as error rather than EOF or full success.
# Проверить precise prefixes, остановку readn на partial refusal и no-progress
# write как error, а не EOF или полный успех. FD ownership ends with this frame.
init()
{
    sys = load Sys Sys->PATH;
    input := sys->open("view/read", Sys->OREAD);
    output := sys->open("view/write", Sys->OWRITE);
    check(input != nil && output != nil, "chunk provider opens");
    bytes := array[] of {byte 'X', byte 'X', byte 'X', byte 'X', byte 'X'};
    check(sys->readn(input, bytes, 5) == 2 && string bytes == "ABXXX", "readn partial refusal discarded/replayed bytes");
    check(sys->read(input, bytes, 5) == -1 && string bytes == "ABXXX", "read refusal changed destination");
    check(sys->readn(input, bytes, 5) == 2 && string bytes == "CDXXX", "readn positive prefix plus EOF");
    check(sys->read(input, bytes, 5) == 0, "source EOF lost");
    check(sys->write(output, array[0] of byte, 0) == 0, "zero-size write failed");
    check(sys->write(output, array of byte "ABCDE", 5) == 2, "partial write count discarded");
    check(sys->write(output, array of byte "CDE", 3) == -1, "positive write zero became EOF");
    check(sys->fprint(output, "%s", "CDE") == 1, "formatted short count fabricated");
    check(sys->write(output, array of byte "DE", 2) == 1, "confirmed prefix before uncertain tail lost");
    check(sys->unmount(nil, "view") == 0, "chunk provider unmount");
    sys->print("PASS precise chunk guest readn/read/write/fprint/zero/EOF/uncertain-prefix\n");
}
