# Copyright (c) 2026 bhgv
# EN: Ordinary typed load and calls into an original wide Dis owner.
# RU: Обычные типизированные load и вызовы исходного Dis с big.
implement Client;
include "sys.m";
include "wide.m";

Client: module { init: fn(); };


# EN: Verify full width, exception transport and persistent private data.
# RU: Проверить полный диапазон, исключение и сохранение частных данных.
init()
{
    sys := load Sys Sys->PATH;
    remote := load Wide "/peer/mounted/wide.dis";
    min := -big 9223372036854775807 - big 1;
    max := big 9223372036854775807;
    if (remote == nil || remote->echo(min) != min || remote->echo(max) != max) {
        raise "original wide owner truncated a scalar";
    }
    {
        remote->echo(big 0);
        raise "expected original wide owner exception";
    } exception {
        "remote scalar owner error" =>
            ;
    }
    if (remote->echo(big 1) != big 4) {
        raise "original wide owner did not retain state";
    }
    sys->print("PASS mounted original big Dis\n");
}
