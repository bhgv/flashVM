# Copyright (c) 2026 bhgv
# EN: Limbo callback for short and transport-fragmented binary requests.
# RU: Колбэк Limbo для коротких и разбитых транспортом двоичных запросов.
implement StyxexecHandler;
include "sys.m";
include "styxexec.m";


# EN: Transform the checked binary input and return its selected reply bytes.
# RU: Преобразовать проверенные входные байты и вернуть выбранный ответ.
invoke(path: string, input: array of byte): array of byte
{
    if (path != "/tools/apply") {
        raise "incorrect callback path";
    }
    if (len input == 4 && input[0] == byte 255 && input[1] == byte 40 &&
        input[2] == byte 0 && input[3] == byte 23) {
        return array[] of {byte 23, byte 0, byte 255};
    }
    if (len input == 16384 && input[0] == byte 17 && input[len input-1] == byte 17) {
        input[0] = byte 88;
        input[len input-1] = byte 99;
        return input;
    }
    raise "incorrect FlashVM binary request";
}
