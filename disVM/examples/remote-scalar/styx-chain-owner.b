# Copyright (c) 2026 bhgv
# EN: Execute on B and resolve a separate Styx library on C.
# RU: Выполнить программу на B и загрузить библиотеку Styx с C.
implement Owner;
include "sys.m";

Owner: module { init: fn(); };
include "wide.m";


# EN: Read A's stdin, call C, then acknowledge via A's separate output files.
# RU: Прочитать stdin A, вызвать C и вывести результаты на разные FD A.
init()
{
    sys := load Sys Sys->PATH;
    data := array[4] of byte;
    input := sys->fildes(0);
    if (input == nil || sys->readn(input, data, len data) != len data || string data != "live") {
        raise "chain input mismatch";
    }
    library := load Wide "/libraries/mounted/wide.dis";
    if (library == nil || library->echo(big 4294967296) != big 4294967296 ||
        library->echo(big 1) != big 2) {
        raise "chain library call failed";
    }
    if (sys->fprint(sys->fildes(1), "chain stdout\n") <= 0 ||
        sys->fprint(sys->fildes(2), "chain stderr\n") <= 0) {
        raise "chain output failed";
    }
}
