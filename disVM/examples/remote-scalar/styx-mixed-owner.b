# Copyright (c) 2026 bhgv
# EN: Load a mixed original Dis library and verify both exports share state.
# RU: Загрузить смешанную библиотеку Dis и проверить общее состояние экспортов.
implement Client;
include "sys.m";
include "wide.m";

Client: module { init: fn(); };
Wrong: module { count: fn(delta: big): big; };


# EN: Interleave both widths, checking the import and the owner's private data.
# RU: Чередовать обе ширины и проверять импорт и частные данные владельца.
init()
{
    sys := load Sys Sys->PATH;
    path := "/peer/mounted/wide.dis";
    if ((load Wrong path) != nil) {
        raise "wrong mixed-width import accepted";
    }
    remote := load WideSet path;
    if (remote == nil || remote->count(40) != 40 ||
        remote->echo(big 1) != big 41 ||
        remote->echo(big 9223372036854775807) != big 9223372036854775807 ||
        remote->count(-2) != 40) {
        raise "mixed Dis exports did not share one instance";
    }
    sys->print("PASS mounted original mixed Dis state\n");
}
