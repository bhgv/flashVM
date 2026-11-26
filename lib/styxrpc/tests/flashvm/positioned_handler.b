# Copyright (c) 2026 bhgv
# EN: Serve a sparse file with a two-byte mutable tail and a separate executable operation.
# RU: Обслужить разреженный файл с двумя изменяемыми байтами в конце и отдельным исполнением.
implement StyxnodePositionedHandler;
include "sys.m";
include "styxnodes.m";

Length: con big 4294967304; # Above 32 bits to check the entire offset.
tail0: byte; # Last two writable bytes.
tail1: byte; # Last byte, including binary zero.


# EN: Validate one complete fragment and process bytes within the declared range.
# RU: Проверить один фрагмент и обработать байты в объявленном диапазоне.
transfer(path, operation: string, buffer: array of byte, offset: big): int
{
    if (path != "/tools/positioned") {
        return -1;
    }
    case operation {
    "w" =>
        if (offset == Length-big 2 && len buffer == 2) {
            tail0 = buffer[0];
            tail1 = buffer[1];
            return len buffer;
        }
    "r" =>
        if (offset == Length-big 3 && len buffer == 3) {
            buffer[0] = byte 11;
            buffer[1] = tail0;
            buffer[2] = tail1;
            return len buffer;
        }
        if (offset == big 0 && len buffer == 1) {
            buffer[0] = byte 7;
            return 1;
        }
    "x" =>
        if (offset == big 0 && len buffer == 1 && buffer[0] == byte 99) {
            buffer[0] = byte 88;
            return 1;
        }
    }
    return -1;
}
