# Copyright (c) 2026 bhgv
# EN: Confirm the write prefixes and return a counted binary reply for both other operations.
# RU: Подтвердить префиксы записи и вернуть двоичный ответ с длиной для двух других операций.
implement StyxnodeHandler;
include "sys.m";
include "styxnodes.m";

tail: int; # Set after the transport retries the unacknowledged suffix.
ended: int; # Set when a second read asks for the missing suffix.


# EN: Check the operation, path and payload, returning the accepted byte count.
# RU: Проверить операцию, путь и данные, вернуть число принятых байтов.
transfer(path, operation: string, buffer: array of byte): int
{
    if (path != "/tools/mail") {
        return -1;
    }
    case operation {
    "w" =>
        if (len buffer == 4 && buffer[0] == byte 255 && buffer[1] == byte 0 &&
            buffer[2] == byte 42 && buffer[3] == byte 23) {
            return 2;
        }
        if (len buffer == 2 && buffer[0] == byte 42 && buffer[1] == byte 23) {
            tail = 1;
            return len buffer;
        }
    "r" =>
        if (len buffer == 4 && tail && buffer[0] == byte 0 && buffer[3] == byte 0) {
            buffer[0] = byte 65;
            buffer[1] = byte 0;
            return 2;
        }
        if (len buffer == 2 && tail && buffer[0] == byte 0 && buffer[1] == byte 0) {
            ended = 1;
            return 0;
        }
    "x" =>
        if (len buffer == 3 && ended && buffer[0] == byte 9 && buffer[1] == byte 0 &&
            buffer[2] == byte 8) {
            buffer[0] = byte 7;
            buffer[1] = byte 255;
            return 2;
        }
    }
    return -1;
}
