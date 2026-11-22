# Copyright (c) 2026 bhgv
# EN: Check that C has the same caller grants as the preceding library.
# RU: Проверить, что C получил те же разрешения клиента, что и предыдущая библиотека.
implement Chainfd;
include "chain-fd.m";
include "sys.m";


# EN: Check open modes and the shared read cursor of A's original descriptors.
# RU: Проверить права и общую позицию исходных дескрипторов A.
forward(input, output: int): int
{
    sys := load Sys Sys->PATH;
    if (input == -1 && output == -1) {
        parked := sys->open("/.prog/.self/.vfs/chain-parked", Sys->OWRITE);
        data := array[] of {byte 0, byte 0, byte 0, byte 1};
        if (parked == nil || sys->write(parked, data, len data) != len data) {
            raise "cannot report nested library entry";
        }
        sys->sleep(30000);
        raise "cancelled library unexpectedly resumed";
    }
    if (input == -2 && output == -2) {
        return 29;
    }
    inpath := sys->sprint("/.prog/.self/.fd/%d", input);
    outpath := sys->sprint("/.prog/.self/.fd/%d", output);
    in := sys->open(inpath, Sys->OREAD);
    out := sys->open(outpath, Sys->OWRITE);
    data := array[4] of byte;
    if (in == nil || out == nil || sys->open(inpath, Sys->OWRITE) != nil ||
        sys->open(outpath, Sys->OREAD) != nil || sys->readn(in, data, len data) != len data ||
        string data != "live" || sys->write(out, data, len data) != len data) {
        raise "nested library lost descriptor identity, mode, or cursor";
    }
    marker := sys->open("/.prog/.self/.vfs/chain-stage", Sys->OREAD);
    if (marker == nil || sys->readn(marker, data, len data) != len data || data[3] != byte 42) {
        raise "nested library lost VFS";
    }
    chain_readonly_var(sys);
    chain_var(sys, 24, 25);
    return 73;
}

include "chain-var.b";
