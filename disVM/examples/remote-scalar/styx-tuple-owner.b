# Copyright (c) 2026 bhgv
# EN: Call the original remote tuple implementation through ordinary load.
# RU: Вызвать исходную удалённую реализацию кортежа через обычный load.
implement Client;
include "sys.m";
include "tuple.m";

Client: module { init: fn(); };


# EN: Check signed width, remote exception and state of the linked owner.
# RU: Проверить знаковую ширину, исключение и состояние экземпляра.
init()
{
    sys := load Sys Sys->PATH;
    owner := load Tuple "/peer/mounted/tuple.dis";
    if (owner == nil) {
        raise "could not load original tuple module";
    }
    min := -big 9223372036854775807 - big 1;
    max := big 9223372036854775807;
    (first, wide) := owner->transform((-2147483647, min));
    if (first != 1 || wide != min) {
        raise "tuple owner lost its first argument";
    }
    {
        owner->transform((0, big 0));
        raise "expected remote tuple exception";
    } exception {
        "remote scalar owner error" =>
            ;
    }
    (third, last) := owner->transform((2147483647, max));
    if (third != 3 || last != max) {
        raise "tuple owner lost state or result";
    }
    sys->print("PASS mounted original tuple Dis\n");
}
