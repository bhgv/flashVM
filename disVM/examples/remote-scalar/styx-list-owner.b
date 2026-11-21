# Copyright (c) 2026 bhgv
# EN: Verify that ordinary load calls the original Dis library with all three arguments.
# RU: Проверка обычного load исходной библиотеки Dis с тремя аргументами.
implement Client;
include "sys.m";
include "blend.m";

Client: module { init: fn(); };


# EN: Exercise nesting, order, signed widths and continued use after a remote error.
# RU: Проверить вложенность, порядок, ширину знаковых чисел и вызов после ошибки.
init()
{
    sys := load Sys Sys->PATH;
    owner := load Blend "/peer/mounted/blend.dis";
    if (owner == nil) {
        raise "could not load original argument-list module";
    }
    min := -big 9223372036854775807 - big 1;
    max := big 9223372036854775807;
    (count, (narrow, wide)) := owner->blend(40, (-2, min), max);
    if (count != 1 || narrow != 38 || wide != big -1) {
        raise "argument order or nested return was lost";
    }
    {
        owner->blend(0, (7, big 8), big 9);
        raise "expected argument-list exception";
    } exception {
        "remote scalar owner error" =>
            ;
    }
    (count, (narrow, wide)) = owner->blend(-3, (44, max), big 0);
    if (count != 3 || narrow != 41 || wide != max) {
        raise "argument-list owner lost state or signed width";
    }
    sys->print("PASS mounted original argument list Dis\n");
}
