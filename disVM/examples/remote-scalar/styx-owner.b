# Copyright (c) 2026 bhgv
# EN: Verify binary startup data inside the owner DisVM.
# RU: Проверить двоичные стартовые данные внутри DisVM владельца.
implement Owner;
include "sys.m";
include "launchcontext.m";

Owner: module { init: fn(); };


# EN: Read the admitted payload as raw bytes, including the embedded NUL.
# RU: Прочитать принятые стартовые данные вместе с нулевым байтом.
init()
{
    sys := load Sys Sys->PATH;
    context := load Launchcontext Launchcontext->PATH;
    arguments := context->arguments();
    if (len arguments != 3 || arguments[0] != byte 'a' ||
        arguments[1] != byte 0 || arguments[2] != byte 'b') {
        raise "owner startup payload mismatch";
    }
    sys->print("PASS mounted Styx owner Dis init\n");
}
